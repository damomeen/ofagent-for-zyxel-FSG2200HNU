#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
  char *data;
  size_t size;
  int bytesCopied;
  int bytesRemaining;
};


char* readFileToBuffer(char* fileName)
{
    char *source = NULL;
    FILE *fp = fopen(fileName, "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */ }

            /* Allocate our buffer to that size. */
            source = malloc(sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) == 0) { /* Error */ }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if (newLen == 0) {
                fputs("Error reading file", stderr);
            } else {
                source[++newLen] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
    }
    return source;
}

static size_t writeDataCallback(void *contents, size_t size, size_t nmemb, void *stream)
{
    size_t totalSize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) stream;

    printf("*****zawartosc contents: %.*s\n", totalSize, contents);

    mem->data = realloc(mem->data, mem->size + totalSize + 1);
    if (mem->data == NULL) {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      exit(EXIT_FAILURE);
    }

    memcpy(&(mem->data[mem->size]), contents, totalSize);
    mem->size += totalSize;
    mem->data[mem->size] = 0;

    return totalSize;

  //int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  //return written;
}


static size_t readDataCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if (stream) {
        struct MemoryStruct *mem = (struct MemoryStruct*) stream;
        size_t dstBufferSize = size*nmemb;
        //size_t dstBufferSize = 5;
        size_t auxRet = 0;
        if (mem->bytesRemaining > dstBufferSize) {
            //printf("****Przed kopiowaniem:\nmem: %s\n", mem->data + mem->bytesCopied);
            //printf("bytesCopied: %d\n", mem->bytesCopied);
            //printf("bytesRemaining: %d\n", mem->bytesRemaining);
            memcpy(ptr, mem->data + mem->bytesCopied, dstBufferSize);
            mem->bytesCopied += dstBufferSize;
            mem->bytesRemaining = mem->size - mem->bytesCopied;
            //printf("****Po kopiowaniu:\nmem: %s\n", mem->data + mem->bytesCopied);
            //printf("bytesCopied: %d\n", mem->bytesCopied);
            //printf("bytesRemaining: %d\n", mem->bytesRemaining);
            return dstBufferSize;
        } else {
            //printf("****Przed last kopiowaniem:\nmem: %s\n", mem->data + mem->bytesCopied);
            //printf("bytesCopied: %d\n", mem->bytesCopied);
            //printf("bytesRemaining: %d\n", mem->bytesRemaining);
            memcpy(ptr, mem->data + mem->bytesCopied, mem->bytesRemaining);
            auxRet = mem->bytesRemaining;
            mem->bytesCopied += mem->bytesRemaining;
            mem->bytesRemaining = mem->size - mem->bytesCopied;                 //mem->bytesRemaining = 0;
            //printf("****Po last kopiowaniu:\nmem: %s\n", mem->data + mem->bytesCopied);
            //printf("bytesCopied: %d\n", mem->bytesCopied);
            //printf("bytesRemaining: %d\n", mem->bytesRemaining);
            return auxRet;
        }
    }
}

void httpPost(CURL *curlHandle, char *url, struct MemoryStruct *requestBody, struct MemoryStruct *responseBody, struct MemoryStruct *responseHeaders)
{
    if (curlHandle == NULL) curlHandle = curl_easy_init();

    if (curlHandle) {
        curl_easy_setopt(curlHandle, CURLOPT_URL, url);                                 // set URL to get
        curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);                           // no progress meter please
        curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, readDataCallback);           //send all data to this function
        curl_easy_setopt(curlHandle, CURLOPT_READDATA, (void*) requestBody);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeDataCallback);         //send all data to this function
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*) responseBody);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEHEADER,(void*) responseHeaders);

        struct curl_slist *headerList = NULL;
        headerList = curl_slist_append(headerList, "Expect:");
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, NULL);       //The pointed data are NOT copied by the library: as a consequence, they must be preserved by the calling application until the transfer finishes.
        //curl_easy_setopt(curlHandle, CURLOPT_COPYPOSTFIELDS, NULL);     //the original data are copied by the library, allowing the application to overwrite the original data after setting this option.

        CURLcode res;
        res = curl_easy_perform(curlHandle);                                        // get it
        if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_slist_free_all(headerList);
        curl_easy_cleanup(curlHandle);
    }
}

int main(int argc, char **argv)
{
    CURL *curlHandle;
    CURLcode res;
    struct curl_slist *headerList = NULL;
    char *urlString = "http://192.168.201.20:8080/wm/core/controller/switches/json";

    //curl_global_init(CURL_GLOBAL_ALL);
    //curlHandle = curl_easy_init();                                                 //init the curl session

    struct MemoryStruct httpPostBody;
    httpPostBody.data = readFileToBuffer(argv[1]);
    httpPostBody.size = strlen(httpPostBody.data);      //nie potrzeba nulla na koncu dawac
    httpPostBody.bytesCopied = 0;
    httpPostBody.bytesRemaining = httpPostBody.size;

    struct MemoryStruct httpResponseBody;
    struct MemoryStruct httpResponseHeaders;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    httpPost(NULL, urlString, &httpPostBody, &httpResponseBody, &httpResponseHeaders);

    printf("Wczytane body: \n%s\n", httpResponseBody.data);
    printf("Wczytane hedery: \n%s\n", httpResponseHeaders.data);

    free(httpPostBody.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    //curl_global_cleanup();
    return 0;

}
