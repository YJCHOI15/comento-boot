#ifndef TASKS_H_
#define TASKS_H_

void StartDefaultTask(void *argument);
void StartI2CTask(void *argument);
void StartSPITask(void *argument);
void StartCANTask(void *argument);
void StartUARTTask(void *argument);
void StartADCTask(void *argument); 


#endif /* TASKS_H_ */