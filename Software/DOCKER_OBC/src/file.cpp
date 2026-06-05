#include "file.h"



void FILE_Init()
{

    //Initialise and verify SD card functionality
    pinMode(SD_CS_PIN, OUTPUT);
    pinMode(SD_SW_PIN, INPUT);
    FILE_checkSDCard();
}


/**
 * @brief Initialise file metadata and calculate number of chunks in data file. 
 * 
 * @param hfile Pointer to the file handler for the file being initialised. The metadata will be stored in the metadata field of the struct, and the file will be opened and stored in the file field of the struct.
 * 
 * @retval True if the metadata was initialised successfully and the file was opened successfully.
*/
bool FILE_initialiseMetadata(FILE_Handler_t* hfile)
{

    // Read the file metadata. If there is no metadata, create a new file and write the default metadata to the SD card. 
    // This will create the file if it doesn't exist, or overwrite it if it does.
    if (FILE_readMetadata(hfile))
    {
        Serial.println("Successfully read File Metadata.");
    }
    else
    {
        Serial.println("File metadata doesn't exist. Creating new metadata file.");
        FILE_writeMetadata(hfile);
    }

    //Open the data file and calculate the number of chunks (this overwrites the metadata declared value but I think its a safer way of doing it)
    hfile->file = SD.open(hfile->file_name, FILE_READ);  
    if (!hfile->file)
    {
        Serial.println("Failed to open file: " + String(hfile->metadata_file_name));
        //TODO: Set OBC fault for failed SD card
        return false;
    }
    uint32_t bytes = hfile->file.size();
    Serial.println("Filesize: " + String(bytes));
    hfile->metadata.num_chunks = bytes / hfile->metadata.chunk_size;
    hfile->file.close();
    return true;
}




/**
 * @brief Read file metadata struct from SD card
 *
 * Reads specified metadata file from the SD card and copies it into RAM as a FILE_Metadata_t struct
 *
 * @param hfile String pointer to the file handler for the file being read. The metadata will be stored in the metadata field of the struct.
 *
 * @return True if the metadata was read succesffully.
 * @return False if the SD file could not be opened or the specified metadata file doesn't exist
 *       
 */
bool FILE_readMetadata(FILE_Handler_t* hfile)
{
    if (hfile== NULL)
    {
        Serial.println("Invalid file pointer");
        return false;
    }

    if (!SD.exists(hfile->metadata_file_name))
    {
        Serial.print("ERROR: ");
        Serial.print(hfile->metadata_file_name);
        Serial.println(" not found.");
        return false;
    }

    File file = SD.open(hfile->metadata_file_name, FILE_READ);

    if (!file)
    {
        Serial.println("Failed to open file metadata");
        return false;
    }

    size_t bytes_read = file.read((uint8_t *)(&hfile->metadata), sizeof(FILE_Metadata_t));
    file.close();

    if (bytes_read != sizeof(FILE_Metadata_t))
    {
        Serial.println("File metadata read incomplete");
        return false;
    }

    Serial.print("Successfully read ");
    Serial.println(hfile->metadata_file_name);

    return true;
}



/**
 * @brief Write metadata struct to SD Card
 *
 * Copies FILE_Metadata_t struct from RAM into SD card.
 *
 * @param filename String pointer to the metadata file handler for the file being written. The metadata will be taken from the metadata field of the struct.
 *
 * @return True if the metadata was written succesffully.
 * @return False if the SD file could not be opened or written
 *       
 */
bool FILE_writeMetadata(FILE_Handler_t* hfile)
{
    File file = SD.open(hfile->metadata_file_name, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file metadata");
        return false;
    }

    if (!file.seek(0))
    {
        Serial.println("Failed to seek file metadata");
        file.close();
        return false;
    }
    size_t written = file.write((const uint8_t *)(&hfile->metadata), sizeof(FILE_Metadata_t));
    file.close();
    if (written != sizeof(FILE_Metadata_t))
    {
        Serial.println("Failed to complete file metadata");
    }
    return true;
}






/**
 * @brief Read a chunk from the SD card. 
 * 
 * @param hfile Pointer to the file handler for the file being read.
 * @param chunk Buffer to store read data
 *
 * @return Number of bytes read.
 *       
 */
size_t FILE_read(FILE_Handler_t *hfile, uint8_t* chunk)
{
    uint32_t offset = hfile->metadata.read_ptr * hfile->metadata.chunk_size;
    if (!hfile->file.seek(offset))
    {
        return 0;
    }
    size_t bytes_read = hfile->file.read((uint8_t *)chunk, hfile->metadata.chunk_size);
    return bytes_read;
}

/**
 * @brief Write a chunk to the SD card. 
 * 
 * @param hfile Pointer to the file handler for the file being written to.
 * @param chunk 
 *
 * @return Number of bytes written.
 *       
 */
size_t FILE_write(FILE_Handler_t *hfile, uint8_t* chunk, uint32_t chunk_size)
{
    size_t bytes_written = hfile->file.write(chunk, chunk_size);
    Serial.println("Bytes written: " + String(bytes_written));
    hfile->file.flush();
    hfile->metadata.num_chunks++;
    return bytes_written;
}




/**
 * @brief Check the SD card is inserted and works
 *
 * 
 * @return True if the SD card is plugged in and can responds 
 * @return False if the SD file is not inserted or not responding.
 *       
 */
bool FILE_checkSDCard(void)
{
    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println("SD card initialisation failed!");

        if (digitalRead(SD_SW_PIN))
        {
            Serial.println("No SD Card present");
        }

        return false;
    }

    Serial.println("SD Card Initialised!");
    return true;
}

/**
 * @brief Open the file to read or write to it.
 *
 * If the file does not exist, it is created.
 *
 * @param hfile Pointer to the file handler for the file being opened.
 * @param read_write Enum value specifying whether to open the file for reading or writing.
 * 
 * @return True if the file was opened successfully
 * @return False if there was an error opening or creating the file
 */
bool FILE_open(FILE_Handler_t *hfile, FILE_OpenState_t read_write)
{
    if (hfile == nullptr || hfile->file_name == nullptr)
    {
        Serial.println("Invalid file handler");
        return false;
    }

    // Create the file if it does not already exist
    if (!SD.exists(hfile->file_name))
    {
        File new_file = SD.open(hfile->file_name, FILE_WRITE);

        if (!new_file)
        {
            Serial.println("Failed to create file: " + String(hfile->file_name));
            return false;
        }

        new_file.close();
    }

    if (read_write == FILE_OPEN_FOR_READ)
    {
        hfile->file = SD.open(hfile->file_name, FILE_READ);
    }
    else if (read_write == FILE_OPEN_FOR_WRITE)
    {
        hfile->file = SD.open(hfile->file_name, FILE_WRITE);
    }
    else
    {
        Serial.println("Invalid file open state");
        return false;
    }

    if (!hfile->file)
    {
        Serial.println("Failed to open file: " + String(hfile->file_name));
        return false;
    }

    hfile->read_write = read_write;
    return true;
}

/**
 * @brief Clear file data and reset the read pointer and num_chunks in the metadata
 *
 *
 * @param hfile Pointer to the file handler for the file being opened.
 * 
 * @return True if the file was cleared successfully
 * @return False if there was an error clearing the file or updating the metadata
 * 
 */
bool FILE_clear(FILE_Handler_t *hfile)
{
    if (hfile == nullptr || hfile->file_name == nullptr)
    {
        Serial.println("Invalid file handler");
        return false;
    }

    // Create the file if it does not already exist
    if (!SD.exists(hfile->file_name))
    {
        if (!SD.remove(hfile->file_name))
        {
            Serial.println("Failed to clear file");
            return false;
        }
    }

    File file = SD.open(hfile->file_name, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to recreate file");
        return false;
    }
    file.close();
    hfile->metadata.read_ptr = 0;
    hfile->metadata.num_chunks = 0;
    if (!FILE_writeMetadata(hfile))
    {
        Serial.println("Failed to save metadata");
    }
    return false;
}