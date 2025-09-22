#ifndef __API_SPIFFS_H__
#define __API_SPIFFS_H__

/* public function protypes ------------------------------------------------- */
void api_spiffs_init(void);
bool spiffs_readfile(const char *path, uint16_t size, char *buffer);

#endif /* __API_SPIFFS_H__ */