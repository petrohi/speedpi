#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/libinput_drv.h"
#include "lvgl/lvgl.h"
#include <pthread.h>

#include "cjson/cJSON.h"

static pthread_mutex_t lvgl_mutex;

static void hal_init(void);
static void *tick_thread_handler(void *data);
static void *speedtest_thread_handler(void *data);

static void btn_event_cb(lv_obj_t *btn, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    pthread_t speedtest_thread;
    int err =
        pthread_create(&speedtest_thread, NULL, &speedtest_thread_handler, NULL);
    if (err)
    {
    }
  }
}

static lv_obj_t *ping_bar = NULL;
static lv_obj_t *upload_bar = NULL;
static lv_obj_t *download_bar = NULL;

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  lv_init();

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  hal_init();

  lv_obj_t *btn = lv_btn_create(lv_scr_act(), NULL);
  lv_btn_toggle(btn);
  lv_obj_t *label = lv_label_create(btn, NULL);
  lv_label_set_text(label, "Button");

  lv_obj_set_event_cb(btn, btn_event_cb);

  ping_bar = lv_bar_create(lv_scr_act(), NULL);
  lv_obj_set_size(ping_bar, 100, 20);
  lv_obj_align(ping_bar, NULL, LV_ALIGN_IN_LEFT_MID, 0, 0);
  lv_bar_set_anim_time(ping_bar, 100);
  lv_bar_set_value(ping_bar, 0, LV_ANIM_ON);

  download_bar = lv_bar_create(lv_scr_act(), NULL);
  lv_obj_set_size(download_bar, 100, 20);
  lv_obj_align(download_bar, NULL, LV_ALIGN_CENTER, 0, 0);
  lv_bar_set_anim_time(download_bar, 100);
  lv_bar_set_value(download_bar, 0, LV_ANIM_ON);

  upload_bar = lv_bar_create(lv_scr_act(), NULL);
  lv_obj_set_size(upload_bar, 100, 20);
  lv_obj_align(upload_bar, NULL, LV_ALIGN_IN_RIGHT_MID, 0, 0);
  lv_bar_set_anim_time(upload_bar, 100);
  lv_bar_set_value(upload_bar, 0, LV_ANIM_ON);

  pthread_mutex_init(&lvgl_mutex, NULL);

  while (1)
  {
    pthread_mutex_lock(&lvgl_mutex);
    lv_task_handler();
    pthread_mutex_unlock(&lvgl_mutex);
    usleep(5000);
  }
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics
 * library
 */
static void hal_init(void)
{
  /* Use the 'monitor' driver which creates window on PC's monitor to simulate a
   * display*/
  fbdev_init();

  /*Create a display buffer*/
  static lv_disp_buf_t disp_buf1;
  static lv_color_t buf1_1[LV_HOR_RES_MAX * 120];
  lv_disp_buf_init(&disp_buf1, buf1_1, NULL, LV_HOR_RES_MAX * 120);

  /*Create a display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv); /*Basic initialization*/
  disp_drv.buffer = &disp_buf1;
  disp_drv.flush_cb = fbdev_flush;
  lv_disp_drv_register(&disp_drv);

  /* Add the mouse as input device
   * Use the 'mouse' driver which reads the PC's mouse*/
  libinput_init();
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv); /*Basic initialization*/
  indev_drv.type = LV_INDEV_TYPE_POINTER;

  /*This function will be called periodically (by the library) to get the mouse
   * position and state*/
  indev_drv.read_cb = libinput_read;
  lv_indev_drv_register(&indev_drv);

  /* Tick init.
   * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about
   * how much time were elapsed Create an SDL thread to do this*/
  pthread_t tick_thread;
  int err = pthread_create(&tick_thread, NULL, &tick_thread_handler, NULL);
  if (err)
  {
  }
}

/**
 * A task to measure the elapsed time for LVGL
 * @param data unused
 * @return never return
 */
static void *tick_thread_handler(void *data)
{
  (void)data;

  while (1)
  {
    usleep(5000);   /*Sleep for 5 millisecond*/
    lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
  }

  return NULL;
}

static void handle_ping(cJSON *item)
{
  cJSON *progress = cJSON_GetObjectItemCaseSensitive(item, "progress");

  if (cJSON_IsNumber(progress))
  {
    lv_bar_set_value(ping_bar, (int)(progress->valuedouble * 100), LV_ANIM_ON);
  }
}

static void handle_download(cJSON *item)
{
  cJSON *progress = cJSON_GetObjectItemCaseSensitive(item, "progress");

  if (cJSON_IsNumber(progress))
  {
    lv_bar_set_value(download_bar, (int)(progress->valuedouble * 100), LV_ANIM_ON);
  }
}

static void handle_upload(cJSON *item)
{
  cJSON *progress = cJSON_GetObjectItemCaseSensitive(item, "progress");

  if (cJSON_IsNumber(progress))
  {
    lv_bar_set_value(upload_bar, (int)(progress->valuedouble * 100), LV_ANIM_ON);
  }
}

static void handle_test_start(cJSON *json)
{
  lv_bar_set_value(ping_bar, 0, LV_ANIM_ON);
  lv_bar_set_value(download_bar, 0, LV_ANIM_ON);
  lv_bar_set_value(upload_bar, 0, LV_ANIM_ON);
}

static void handle_result(cJSON *json)
{
  lv_bar_set_value(ping_bar, 0, LV_ANIM_ON);
  lv_bar_set_value(download_bar, 0, LV_ANIM_ON);
  lv_bar_set_value(upload_bar, 0, LV_ANIM_ON);
}

static void handle_json(cJSON *json)
{
  cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");

  if (cJSON_IsString(type))
  {
    if (strcmp(type->valuestring, "testStart") == 0)
    {
      handle_test_start(json);
    }
    else if (strcmp(type->valuestring, "ping") == 0)
    {
      cJSON *ping = cJSON_GetObjectItemCaseSensitive(json, "ping");
      if (cJSON_IsObject(ping))
      {
        handle_ping(ping);
      }
    }
    else if (strcmp(type->valuestring, "download") == 0)
    {
      cJSON *download = cJSON_GetObjectItemCaseSensitive(json, "download");
      if (cJSON_IsObject(download))
      {
        handle_download(download);
      }
    }
    else if (strcmp(type->valuestring, "upload") == 0)
    {
      cJSON *upload = cJSON_GetObjectItemCaseSensitive(json, "upload");
      if (cJSON_IsObject(upload))
      {
        handle_upload(upload);
      }
    }
    else if (strcmp(type->valuestring, "result") == 0)
    {
      handle_result(json);
    }
  }
}

static void *speedtest_thread_handler(void *data)
{
  (void)data;

  FILE *file;
  char line[1024];

  file = popen("/usr/bin/speedtest -fjson -pyes", "r");
  if (file == NULL)
  {
    printf("Failed to run speedtest\n");
    exit(1);
  }

  while (fgets(line, sizeof(line), file) != NULL)
  {
    cJSON *json = cJSON_Parse(line);
    printf("%s", line);

    if (cJSON_IsObject(json))
    {
      pthread_mutex_lock(&lvgl_mutex);
      handle_json(json);
      pthread_mutex_unlock(&lvgl_mutex);
      cJSON_Delete(json);
    }
  }

  pclose(file);

  return NULL;
}