#ifndef TASK_H
#define TASK_H


#ifdef ESP_PLATFORM
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#include <pthread.h>
#include <string>

class Task
{
private:
    int stackSize = 0;
    std::string threadName = "";
    int pinToCore = 0;
#ifdef ESP_PLATFORM
	bool running = false;
#endif	
public:
   Task(int stackSize, std::string threadName, int pinToCore) {
       this->threadName = threadName;
       this->pinToCore = pinToCore;
       this->stackSize = stackSize;
   }
   virtual ~Task() {}

   bool startTask()
   {
#ifdef ESP_PLATFORM
	  /*
      esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
      cfg.stack_size = this->stackSize;
      cfg.inherit_cfg = true;
      cfg.thread_name = this->threadName.c_str();
      cfg.pin_to_core = this->pinToCore;
      esp_pthread_set_cfg(&cfg);
	  */
	  xTaskBuffer = (StaticTask_t*) heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
	  xStack = (StackType_t*) heap_caps_malloc(this->stackSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	  return (xTaskCreateStatic(taskEntryFunc, this->threadName.c_str(), this->stackSize, this, ESP_TASK_PRIO_MIN + 2, xStack, xTaskBuffer) != NULL);
#else
      return (pthread_create(&_thread, NULL, taskEntryFunc, this) == 0);
#endif	  
   }
   void waitForTaskToReturn()
   {
#ifdef ESP_PLATFORM	   
	  while (running) 
	  {
	    vTaskDelay(100 / portTICK_PERIOD_MS);
	  }	
	  heap_caps_free(xTaskBuffer);
	  heap_caps_free(xStack);
#else
      (void)pthread_join(_thread, NULL);
#endif
   }

protected:
   virtual void runTask() = 0;

private:
#ifdef ESP_PLATFORM
   StaticTask_t *xTaskBuffer;
   StackType_t *xStack;
   static void taskEntryFunc(void *This)
   {
	  Task *self = (Task *)This;
	  self->running = true;
      self->runTask();
	  self->running = false;
      vTaskDelete(NULL);
   }
#else
   static void *taskEntryFunc(void *This)
   {   
      ((Task *)This)->runTask();
      return NULL;
   }
#endif   

   pthread_t _thread;
};

#endif
