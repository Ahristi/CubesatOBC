#ifndef __FILE_H__
#define __FILE_H__
#include <stdint.h>
#include <SD.h>
//-------------DEFINES--------------
#define SD_CS_PIN 10
#define SD_SW_PIN 2
#define MAX_CHUNK_SIZE 192
#define MAX_FILE_NAME_SIZE 192



#define WOD_BUFFER_BYTES (1024UL * 1024UL * 1024UL) //1GiB
#define WOD_RECORD_BYTES sizeof(LOGGING_Record_t)
#define WOD_MAX_RECORDS  (WOD_BUFFER_BYTES / WOD_RECORD_BYTES)

             

#define RESULT_META_FILE_NAME           "/result_metadata.bin"
#define RESULT_DATA_FILE_NAME           "/result.bin"

#define EXPERIMENT_META_FILE_NAME       "/experiment_metadata.bin"
#define EXPERIMENT_DATA_FILE_NAME       "/experiment.bin"





//-------------Typedefs and Enums--------------
typedef enum{
    FILE_CLOSED,
    FILE_OPEN_FOR_READ,
    FILE_OPEN_FOR_WRITE
}FILE_OpenState_t;


typedef struct __attribute__((packed)) {
    uint8_t ID;
    uint32_t chunk_size;
    uint32_t num_chunks;
    uint32_t max_chunks;
    uint32_t read_ptr;
}FILE_Metadata_t;

typedef struct __attribute__((packed)) {
    uint8_t  file_id;
    uint32_t chunk_index;
    uint8_t  data[MAX_CHUNK_SIZE];
    uint8_t length;
}FILE_ChunkPayload_t;

typedef struct {
    char metadata_file_name[MAX_FILE_NAME_SIZE];
    char file_name[MAX_FILE_NAME_SIZE];
    FILE_Metadata_t metadata;
    File file;
    uint32_t file_size;
    FILE_OpenState_t read_write;
}FILE_Handler_t;

extern FILE_Handler_t wod;
extern FILE_Handler_t results;
extern FILE_Handler_t experiment;

//Create an abstracted struct for a file with the read pointer, and the number of chunks 
//We can use this for both Comms and payload data transfer.


//-------------Function Prototypes--------------
void FILE_Init();
bool FILE_initialiseMetadata(FILE_Handler_t* hfile);
bool FILE_readMetadata(FILE_Handler_t* hfile);
bool FILE_writeMetadata(FILE_Handler_t* hfile);
size_t FILE_write(FILE_Handler_t *hfile, uint8_t* chunk, uint32_t chunk_size);
size_t FILE_read(FILE_Handler_t *hfile, uint8_t* chunk);
bool FILE_checkSDCard(void);
bool FILE_open(FILE_Handler_t *hfile, FILE_OpenState_t read_write);
#endif