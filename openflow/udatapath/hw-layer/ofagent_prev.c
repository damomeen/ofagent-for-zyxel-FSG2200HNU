#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

    //httpGet(CURL *curlHandle, char *url, char *requestBody, char *responseBody, char *responseHeaders)
    //httpPut(CURL *curlHandle, char *url, char *requestBody, char *responseBody, char *responseHeaders)
    //httpPost(CURL *curlHandle, char *url, char *requestBody, char *responseBody, char *responseHeaders)

static size_t writeDataCallback(void *contents, size_t size, size_t nmemb, void *stream)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) stream;

    printf("*****zawartosc contents: %.*s\n", realsize, contents);

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      exit(EXIT_FAILURE);
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;

  //int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  //return written;
}

void httpGet(CURL *curlHandle, char *url, struct MemoryStruct *responseBody, struct MemoryStruct *responseHeaders)
{
    if (curlHandle == NULL) curlHandle = curl_easy_init();

    if (curlHandle) {
        curl_easy_setopt(curlHandle, CURLOPT_URL, url);                                 // set URL to get
        curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);                           // no progress meter please
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeDataCallback);         //send all data to this function
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*) responseBody);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEHEADER,(void*) responseHeaders);

        CURLcode res;
        res = curl_easy_perform(curlHandle);                                            // get it
        if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curlHandle);
    }
}

int main(void)
{
    CURL *curlHandle;
    CURLcode res;
    char *urlString = "http://192.168.201.20/svn/floodlight/branches/floodlight/";

    //curl_global_init(CURL_GLOBAL_ALL);
    //curlHandle = curl_easy_init();                                                 //init the curl session

    struct MemoryStruct httpResponseBody;
    struct MemoryStruct httpResponseHeaders;
    httpResponseBody.memory = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.memory = malloc(1);
    httpResponseHeaders.size = 0;

    httpGet(NULL, urlString, &httpResponseBody, &httpResponseHeaders);

    printf("Wczytane body: \n%s\n", httpResponseBody.memory);
    printf("Wczytane hedery: \n%s\n", httpResponseHeaders.memory);

    if (httpResponseBody.memory) free(httpResponseBody.memory);
    if (httpResponseHeaders.memory) free(httpResponseHeaders.memory);

    //curl_global_cleanup();
    return 0;

}
