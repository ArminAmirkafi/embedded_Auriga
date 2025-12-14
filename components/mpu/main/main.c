void app_main(void)
{
    enc_queue = xQueueCreate(32, sizeof(enc_event_t));
    enc_init();
    xTaskCreate(enc_task,"enc_task",4096,NULL,10,NULL);
}
