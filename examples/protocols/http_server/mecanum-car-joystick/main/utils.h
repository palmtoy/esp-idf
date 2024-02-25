#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_LOOP_PRIORITY  2
#define CAR_CMD_LENGTH      16
#define G_X_QUEUE_LENGTH    128
#define G_TASK_STACK_SIZE   (1024 * 4) // 4 KiB


static xQueueHandle G_X_QUEUE_OBJ = NULL;

#ifdef __cplusplus
}
#endif
