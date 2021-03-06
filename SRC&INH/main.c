#include "main.h" 
#include "hw.h"
#include "hal.h"
#include "delay.h" 
#include "task.h"
#include "semphr.h"
#include "my_usb.h"
#include "DL212_easy_mode.h"
 #include "math.h"
//Modbus Includes
#include "usart_idle_dma_modbus.h"   
#include "portfunction.h"
//lua includes
#include "lua_task.h"
 
TaskHandle_t Task_Start_Handler; 
TaskHandle_t Task1_Handler; 
TaskHandle_t Task2_Handler; 
TaskHandle_t Task3_Handler; 
TaskHandle_t QueryTask_Handler; 

QueueHandle_t xQueue; 
SemaphoreHandle_t BinarySemaphore_MB; 
SemaphoreHandle_t BinarySemaphore_USB;  
SemaphoreHandle_t xSemaphore; 

TimerHandle_t OneShotTimer_Handle; 

int main(void){ 
	unsigned int i; 
  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //设置系统中断优先级分组4	 	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE); 
	USART1_Config(9600);
	delay_init(); //延时函数初始化   
	//delay_ms(3000); 
  //DBGMCU_Config(DBGMCU_SLEEP,ENABLE);
	//创建开始任务
  xTaskCreate((TaskFunction_t )Task_Start,           //任务函数
              (const char*    )"start_task",         //任务名称
              (uint16_t       )32,                   //任务堆栈大小
              (void*          )NULL,                 //传递给任务函数的参数
              (UBaseType_t    )1,                    //任务优先级
              (TaskHandle_t*  )&Task_Start_Handler); //任务句柄              
  vTaskStartScheduler();          //开启任务调度

  return 0;
} 

//开始任务任务函数
void Task_Start(void *pvParameters){
  taskENTER_CRITICAL();           //进入临界区
  xQueue = xQueueCreate(10,sizeof(char));
	BinarySemaphore_MB = xSemaphoreCreateBinary(); 
  BinarySemaphore_USB = xSemaphoreCreateBinary(); 
 	xSemaphore = xSemaphoreCreateMutex(); 
  if(xQueue==NULL || BinarySemaphore_MB==NULL || BinarySemaphore_USB==NULL || xSemaphore==NULL){
	  while(1); 
	} 
	My_USB_Init(); 
	psSE_FUNC->init(); 
	psFram_Func->init(); 
  psFlash_Func->init();  
	psSW12_Func->init();//SW12控制口初始化
	psC_Pulse_Func->init(0);//C1口默认初始化为脉冲测量
	psC_Pulse_Func->init(1);//C2口默认初始化为脉冲测量  
 // sDL212_State.battery = psSE_FUNC->bat_read();
 // psSE_FUNC->vref_read(); 
  I2C_RTC_Init();
  TIM2_TI2FP2_Init();//PSW-----PA1(TIM2_CH2)
  TIM3_ETR_Init();   //C2------PD2(TIM3_ETR)
  TIM9_TI2FP2_Init();//C1------PA3(TIM9_CH2) 
	TIM5_TI1FP1_Init();//F_Mea---PA0(TIM5_CH1)  
	/*SDI12_C1_SEND_DISABLE();
	psSDI12_Func->init(0);
	psSDI12_Func->init(1);
	psC_RS232_Func->init(0,0);*/  
	OneShotTimer_Handle = xTimerCreate((const char*)"OneShotTimer",
		                                 (TickType_t)200,//200ms
	                                   (UBaseType_t)pdFALSE,
	                                   (void*)2,
															       (TimerCallbackFunction_t)OneShotCallback);
    xTaskCreate((TaskFunction_t )Task1, 
                (const char*    )"task1", 
                (uint16_t       )128, 
                (void*          )NULL, 
                (UBaseType_t    )1,	
                (TaskHandle_t*  )&Task1_Handler); 

    xTaskCreate((TaskFunction_t )Task2,
                (const char*    )"task2",   
                (uint16_t       )128, 
                (void*          )NULL,
                (UBaseType_t    )1,
                (TaskHandle_t*  )&Task2_Handler); 
	  xTaskCreate((TaskFunction_t )Task3,     
                (const char*    )"task3",   
                (uint16_t       )512,  
                (void*          )NULL,
                (UBaseType_t    )1,
                (TaskHandle_t*  )&Task3_Handler); 
		/*xTaskCreate((TaskFunction_t )Query_Task,     
                (const char*    )"query_task",   
                (uint16_t       )128,
                (void*          )NULL,
                (UBaseType_t    )3,
                (TaskHandle_t*  )&QueryTask_Handler);*/ 						
    vTaskDelete(Task_Start_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}

unsigned int DL212_EasyMode_Scan_Count=0; 
void Task1(void *pvParameters){ 
	TickType_t xLastWakeTime; 
	const TickType_t xPeriod=pdMS_TO_TICKS(1000); 
	
	DL212_EasyMode_Init();
	xLastWakeTime = xTaskGetTickCount(); 
  while(1){ 
    vTaskDelayUntil(&xLastWakeTime,xPeriod); 
		if(DL212_EasyMode){
      DL212_EasyMode_Scan(); 
      DL212_EasyMode_Scan_Count++; 
			memcpy(usRegInputBuf,sEMData.value,28);
		}
	} 
} 

void Task2(void *pvParameters){ 
	BaseType_t err=pdFALSE; 

  while(1){ 
    if(USB_DETECT()){  
      if(bDeviceState == CONFIGURED){ 
        CDC_Receive_DATA();
        if(sUSB_Para.packet_rec){
          DL212_EasyMode_Config(); 
          //USB_Send(sUSB_Para.rx_buf,sUSB_Para.rec_len); 
          sUSB_Para.packet_rec = 0; 
          sUSB_Para.rec_len = 0; 
        } 
      } 
	  } 
		else{ 
		  if(BinarySemaphore_USB != NULL){ 
        if(xSemaphoreTake(BinarySemaphore_USB,portMAX_DELAY) == pdTRUE){ 
	        SystemInit(); 
				} 
			} 
		} 
  } 
} 
 
void Task3(void *pvParameters){
	portTickType xLastWakeTime; 
  unsigned char i=0;
	
  if(sEMData.usart_baudrate){
	  sMBSlave.init(sEMData.usart_baudrate);   
	}
	else{
	  sMBSlave.init(9600); 
	}
  lua_main();
	while(1){ 
	  if(BinarySemaphore_MB != NULL){ 
      if(xSemaphoreTake(BinarySemaphore_MB,portMAX_DELAY) == pdTRUE){ 
				if(1 == sEMData.usart_mode){
					ucMBAddress = sEMData.modbus_address;
				  sMBSlave.poll();
				}
        else{
				  memset(USART_DMA_RxBuf,0,USART1_DMA_Rec_Cnt);
				  USART1_DMA_Rec_Cnt =0;
				}
	    }
      else{
			  vTaskDelay(1000);
			}
    }
	}
} 

char InfoBuffer[1];				//保存信息的数组
void Query_Task(void *pvParameters){
  unsigned int TotalRunTime;
	UBaseType_t ArraySize,x;
	TaskStatus_t *StatusArray;
	
	//第一步：函数uxTaskGetSystemState()的使用
	printf("/********第一步：函数uxTaskGetSystemState()的使用**********/\r\n");
	ArraySize=uxTaskGetNumberOfTasks();		//获取系统任务数量
	StatusArray=pvPortMalloc(ArraySize*sizeof(TaskStatus_t));//申请内存
	if(StatusArray!=NULL)					//内存申请成功
	{
		ArraySize=uxTaskGetSystemState((TaskStatus_t* 	)StatusArray, 	//任务信息存储数组
									   (UBaseType_t		)ArraySize, 	//任务信息存储数组大小
								       (uint32_t*		)&TotalRunTime);//保存系统总的运行时间
		printf("TaskName\t\tPriority\t\tTaskNumber\t\t\r\n");
		for(x=0;x<ArraySize;x++){
			//通过串口打印出获取到的系统任务的有关信息，比如任务名称、
			//任务优先级和任务编号。
			printf("%s\t\t%d\t\t\t%d\t\t\t\r\n",				
					StatusArray[x].pcTaskName,
					(int)StatusArray[x].uxCurrentPriority,
					(int)StatusArray[x].xTaskNumber);	
		}
	}
	vPortFree(StatusArray);	//释放内存
	printf("/**************************结束***************************/\r\n");
	//第二步：函数vTaskGetInfo()的使用
	TaskHandle_t TaskHandle;	
	TaskStatus_t TaskStatus;
	
	printf("/************第二步：函数vTaskGetInfo()的使用**************/\r\n");
	TaskHandle=xTaskGetHandle("task1");			//根据任务名获取任务句柄。
	//获取Task1的任务信息
	vTaskGetInfo((TaskHandle_t	)TaskHandle, 		//任务句柄
				       (TaskStatus_t*	)&TaskStatus, 		//任务信息结构体
				       (BaseType_t	)pdTRUE,			//允许统计任务堆栈历史最小剩余大小
			         (eTaskState	)eInvalid);			//函数自己获取任务运行壮态
	//通过串口打印出指定任务的有关信息。
	printf("任务名:                %s\r\n",TaskStatus.pcTaskName);
	printf("任务编号:              %d\r\n",(int)TaskStatus.xTaskNumber);
	printf("任务壮态:              %d\r\n",TaskStatus.eCurrentState);
	printf("任务当前优先级:        %d\r\n",(int)TaskStatus.uxCurrentPriority);
	printf("任务基优先级:          %d\r\n",(int)TaskStatus.uxBasePriority);
	printf("任务堆栈基地址:        %#x\r\n",(int)TaskStatus.pxStackBase);
	printf("任务堆栈历史剩余最小值:%d\r\n",TaskStatus.usStackHighWaterMark);
	printf("/**************************结束***************************/\r\n");
	//第三步：函数eTaskGetState()的使用	
	eTaskState TaskState;
	char TaskInfo[10];
	printf("/***********第三步：函数eTaskGetState()的使用*************/\r\n");
	TaskHandle=xTaskGetHandle("query_task");		//根据任务名获取任务句柄。
	TaskState=eTaskGetState(TaskHandle);			//获取query_task任务的任务壮态
	memset(TaskInfo,0,10);						
	switch((int)TaskState)
	{
		case 0:
			sprintf(TaskInfo,"Running");
			break;
		case 1:
			sprintf(TaskInfo,"Ready");
			break;
		case 2:
			sprintf(TaskInfo,"Suspend");
			break;
		case 3:
			sprintf(TaskInfo,"Delete");
			break;
		case 4:
			sprintf(TaskInfo,"Invalid");
			break;
	}
	printf("任务壮态值:%d,对应的壮态为:%s\r\n",TaskState,TaskInfo);
	printf("/**************************结束**************************/\r\n");
	//第四步：函数vTaskList()的使用	
	printf("/*************第三步：函数vTaskList()的使用*************/\r\n");
	vTaskList(InfoBuffer);							//获取所有任务的信息
	printf("%s\r\n",InfoBuffer);					//通过串口打印所有任务的信息
	printf("/**************************结束**************************/\r\n");
	while(1){
	  vTaskDelay(1000);
		vTaskList(InfoBuffer);							//获取所有任务的信息
	  printf("%s\r\n",InfoBuffer);					//通过串口打印所有任务的信息 
	}
}

int idlehookcnt=0; 
void vApplicationIdleHook(void){
  idlehookcnt++; 
}

int tickhookcnt=0; 
void vApplicationTickHook(void){
  tickhookcnt++; 
}

void OneShotCallback(TimerHandle_t xTimer){ 
	BaseType_t xHigherPriorityTaskWoken, xResult;
	
} 
 
void UserGpio_Config(void){
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB|RCC_AHBPeriph_GPIOC,ENABLE); 
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	GPIOB->BSRRL = GPIO_Pin_5;
 
	//2.5v and ref
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	//se1-se4
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN; 
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; 
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}
  