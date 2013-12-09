/**
 * section: InputOutput
 * synopsis: Output to char buffer
 * purpose: Demonstrate the use of xmlDocDumpMemory
 *          to output document to a character buffer
 * usage: io2
 * test: io2 > io2.tmp && diff io2.tmp $(srcdir)/io2.res
 * author: John Fleck
 * copy: see Copyright for the status of this software.
 */

#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define MAX_PCP 8

typedef struct {
	int priority;
	unsigned int ingressCIR;	//up to 10 000 000 000! - double?
	unsigned int ingressCBS;
	unsigned int ingressEIR;	//up to 10 000 000 000! - double?
	unsigned int ingressEBS;
	unsigned char ingressColorMode;
	unsigned char ingressCouplingFlag;

} eLineTrafficMappingTable;

typedef struct connectionTerminationPoint {
	char* parentTp;
	unsigned short ivid;
	char* portTpRoleState;
	eLineTrafficMappingTable bandwidthProfile;

} connectionTp;

typedef struct {
    char *data;
    size_t size;
    int bytesCopied;
    int bytesRemaining;
} MemoryStruct;


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
    MemoryStruct *mem = (MemoryStruct*) stream;

    //printf("*****zawartosc contents: %.*s\n", totalSize, contents);

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
}


static size_t readDataCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if (stream) {
        MemoryStruct *mem = (MemoryStruct*) stream;
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

void httpDelete(CURL *curlHandle, char *url, MemoryStruct *responseBody, MemoryStruct *responseHeaders)
{
    if (curlHandle == NULL) curlHandle = curl_easy_init();

    if (curlHandle) {
        curl_easy_setopt(curlHandle, CURLOPT_URL, url);                                 // set URL to get
        curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);                           // no progress meter please
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeDataCallback);         //send all data to this function
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*) responseBody);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEHEADER,(void*) responseHeaders);
        curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");

        CURLcode res;
        res = curl_easy_perform(curlHandle);                                            // get it
        if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curlHandle);
    }
}

void httpGet(CURL *curlHandle, char *url, MemoryStruct *responseBody, MemoryStruct *responseHeaders)
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

void httpPut(CURL *curlHandle, char *url, MemoryStruct *requestBody, MemoryStruct *responseBody, MemoryStruct *responseHeaders)
{
    if (curlHandle == NULL) curlHandle = curl_easy_init();

    if (curlHandle) {
        curl_easy_setopt(curlHandle, CURLOPT_URL, url);                             // set URL to get
        curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);                       // no progress meter please
        curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, readDataCallback);       // send all data to this function
        curl_easy_setopt(curlHandle, CURLOPT_READDATA, (void*) requestBody);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeDataCallback);         //send all data to this function
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*) responseBody);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEHEADER,(void*) responseHeaders);

        struct curl_slist *headerList = NULL;
        headerList = curl_slist_append(headerList, "Expect:");
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

        /* enable uploading */
        curl_easy_setopt(curlHandle, CURLOPT_UPLOAD, 1L);

        /* HTTP PUT please */
        curl_easy_setopt(curlHandle, CURLOPT_PUT, 1L);

        curl_easy_setopt(curlHandle, CURLOPT_INFILESIZE_LARGE, (curl_off_t) requestBody->size);

        CURLcode res;
        res = curl_easy_perform(curlHandle);                                        // get it
        if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_slist_free_all(headerList);
        curl_easy_cleanup(curlHandle);
    }
}

void httpPost(CURL *curlHandle, char *url, MemoryStruct *requestBody, MemoryStruct *responseBody, MemoryStruct *responseHeaders)
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
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, requestBody->size);

        CURLcode res;
        res = curl_easy_perform(curlHandle);                                        // get it
        if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_slist_free_all(headerList);
        curl_easy_cleanup(curlHandle);
    }
}

void displayMemory(char *address, int length) {
        int i = 0; //used to keep track of line lengths
        char *line = (char*)address; //used to print char version of data
        unsigned char ch; // also used to print char version of data
        printf("%08X | ", (int)address); //Print the address we are pulling from
        while (length-- > 0) {
                printf("%02X ", (unsigned char)*address++); //Print each char
                if (!(++i % 16) || (length == 0 && i % 16)) { //If we come to the end of a line...
                        //If this is the last line, print some fillers.
                        if (length == 0) { while (i++ % 16) { printf("__ "); } }
                        printf("| ");
                        while (line < address) {  // Print the character version
                                ch = *line++;
                                printf("%c", (ch < 33 || ch == 255) ? 0x2E : ch);
                        }
                        // If we are not on the last line, prefix the next line with the address.
                        if (length > 0) { printf("\n%08X | ", (int)address); }
                }
        }
        puts("");
}

char* createRequestBody1(unsigned short pcpToTcMappingTable[], unsigned short tableSize)
{
    xmlNodePtr node1, node2;
    xmlDocPtr xmlDoc;
    xmlChar *xmlbuff;
    char auxBuff[100];
    char* retBuff;
    int buffersize, i;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");                     //create the document
    node1 = xmlNewNode(NULL, BAD_CAST "ns0:modifyEthPtpRequest");
    xmlNewProp(node1, BAD_CAST "xmlns:xsi", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");
    xmlNewProp(node1, BAD_CAST "xmlns:ns0", BAD_CAST "http://www.intunenetworks.com/inx/mgmt/nsp/eth/v1");
    xmlNewProp(node1, BAD_CAST "xsi:schemaLocation", BAD_CAST "http://www.intunenetworks.com/inx/mgmt/nsp/eth/v1 ecs.xsd");

    xmlDocSetRootElement(xmlDoc, node1);
    node2 = xmlNewChild(node1, NULL, BAD_CAST "ns0:modifyEthPtpData", NULL);
    node1 = xmlNewChild(node2, NULL, BAD_CAST "ns0:pcpToTcMappingTable", NULL);

    for (i=0; i < tableSize; i++){
        node2 = xmlNewChild(node1, NULL, BAD_CAST "ns0:entry", NULL);
        sprintf(auxBuff, "%d", i);
        xmlNewChild(node2, NULL, BAD_CAST "ns0:priority", BAD_CAST auxBuff);
        sprintf(auxBuff, "%d", pcpToTcMappingTable[i]);
        xmlNewChild(node2, NULL, BAD_CAST "ns0:trafficClass", BAD_CAST auxBuff);
    }

    xmlDocDumpFormatMemory(xmlDoc, &xmlbuff, &buffersize, 1);
    retBuff = malloc(buffersize + 1);
    memcpy(retBuff, (char*) xmlbuff, buffersize);
    retBuff[buffersize] = 0;
    /*
    printf("xmlbuff:\n\n");
    displayMemory((char*) xmlbuff, buffersize + 1);
    printf("retbuff:\n\n");
    displayMemory((char*) retBuff, buffersize + 1);
    */
    xmlFree(xmlbuff);       //free associated memory
    xmlFreeDoc(xmlDoc);
    return retBuff;
}

char* createRequestBody2(unsigned short ivid, connectionTp AEndTp, connectionTp ZEndTp)
{
    xmlNodePtr node1, node2;
    xmlDocPtr xmlDoc;
    xmlChar *xmlbuff;
    char auxBuff[100];
    char* retBuff;
    int buffersize, i;

    xmlDoc = xmlNewDoc(BAD_CAST "1.0");                                     //create document
    node1 = xmlNewNode(NULL, BAD_CAST "ns0:createAndActivateEvplRequest");
    xmlNewProp(node1, BAD_CAST "xmlns:xsi", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");
    xmlNewProp(node1, BAD_CAST "xmlns:ns0", BAD_CAST "http://www.intunenetworks.com/inx/mgmt/nsp/eth/v1");
    xmlNewProp(node1, BAD_CAST "xsi:schemaLocation", BAD_CAST "http://www.intunenetworks.com/inx/mgmt/nsp/eth/v1 ecs.xsd");

    xmlDocSetRootElement(xmlDoc, node1);
    node2 = xmlNewChild(node1, NULL, BAD_CAST "ns0:createEvplData", NULL);
    node1 = xmlNewChild(node2, NULL, BAD_CAST "ns0:evplServiceAttributeParameters", NULL);
    sprintf(auxBuff, "%d", ivid);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:ivid", BAD_CAST auxBuff);

    node1 = xmlNewChild(node2, NULL, BAD_CAST "ns0:createAEndTp", NULL);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:parentTp", AEndTp.parentTp);
    sprintf(auxBuff, "%d", AEndTp.ivid);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:ivid", auxBuff);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:portTpRoleState", AEndTp.portTpRoleState);

    node1 = xmlNewChild(node2, NULL, BAD_CAST "ns0:createZEndTp", NULL);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:parentTp", ZEndTp.parentTp);
    sprintf(auxBuff, "%d", ZEndTp.ivid);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:ivid", auxBuff);
    xmlNewChild(node1, NULL, BAD_CAST "ns0:portTpRoleState", ZEndTp.portTpRoleState);

    xmlDocDumpFormatMemory(xmlDoc, &xmlbuff, &buffersize, 1);
    retBuff = malloc(buffersize + 1);
    memcpy(retBuff, (char*) xmlbuff, buffersize);
    retBuff[buffersize] = 0;

    /*
    printf("xmlbuff:\n\n");
    displayMemory((char*) xmlbuff, buffersize + 1);
    printf("retbuff:\n\n");
    displayMemory((char*) retBuff, buffersize + 1);
    */
    xmlFree(xmlbuff);
    xmlFreeDoc(xmlDoc);
    return retBuff;
}

void processNode(xmlTextReaderPtr reader, char* string)
{
    xmlChar *name, *value;
    static int foundNode = 0;

    if (foundNode) {
        foundNode = 0;
        value = xmlTextReaderValue(reader);
        printf("Value: %s\n", value);
        xmlFree(value);
        return;
    }

    name = xmlTextReaderName(reader);
    if (strcmp(string, (char*) name) == 0 && xmlTextReaderNodeType(reader) == 1) {
        foundNode = 1;
        printf("Name: %s\tType: %d\t", name, xmlTextReaderNodeType(reader));
        xmlFree(name);
        return;
    }
    xmlFree(name);
    return;
}

char* getElementContent2(MemoryStruct *xmlDoc, char *elementName)
{
    xmlTextReaderPtr reader;
    int ret;
    //char *string = "ns0:priority";
    xmlChar *name, *value;
    int foundNode = 0;

    reader = xmlReaderForMemory(xmlDoc->data, xmlDoc->size, "http://www.example.org/", NULL, 0);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            //processNode(reader, string);
            if (foundNode) {
                foundNode = 0;
                value = xmlTextReaderValue(reader);
                //printf("Value: %s\n", value);
                //xmlFree(value);
                xmlFreeTextReader(reader);
                return (char*) value;
            }
            name = xmlTextReaderName(reader);
            if (strcmp(elementName, (char*) name) == 0 && xmlTextReaderNodeType(reader) == 1) {
                foundNode = 1;
                //printf("Name: %s\tType: %d\t", name, xmlTextReaderNodeType(reader));
                //xmlFree(name);
                //break;
            }
            xmlFree(name);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) printf("Failed to parse.\n");
    }
    else printf("Unable to read XML from memory.\n");
    return;
}

char* getElementContent(MemoryStruct *xmlDoc, char *elementName)
{
    xmlTextReaderPtr reader;
    int ret;
    xmlChar *name, *value;

    reader = xmlReaderForMemory(xmlDoc->data, xmlDoc->size, "http://www.example.org/", NULL, 0);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            name = xmlTextReaderName(reader);
            if (strcmp(elementName, (char*) name) == 0 && xmlTextReaderNodeType(reader) == 1) {
                //printf("Name: %s\tType: %d\t", name, xmlTextReaderNodeType(reader));
                ret = xmlTextReaderRead(reader);
                if (ret == 1) {
                    value = xmlTextReaderValue(reader);
                    //printf("Value: %s\n", value);
                    //xmlFree(value);
                    xmlFree(name);
                    xmlFreeTextReader(reader);
                    return (char*) value;
                }
            }
            xmlFree(name);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) printf("Failed to parse.\n");
    }
    else printf("Unable to read XML from memory.\n");
    return;
}

void configurePCPtoTCMappings(char* url, unsigned short pcpToTcMappingTable[], unsigned short tableSize)
{
    MemoryStruct httpPutBody;
    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    httpPutBody.data = createRequestBody1(pcpToTcMappingTable, tableSize);
    httpPutBody.size = strlen(httpPutBody.data);
    httpPutBody.bytesCopied = 0;
    httpPutBody.bytesRemaining = httpPutBody.size;
    printf("requestBody:\n\n%s\n", httpPutBody.data);

    printf("getElementContent:\n");
    char* element = getElementContent(&httpPutBody, "ns0:priority");
    printf("getElementContent: returned %s\n", element);
    free(element);

    printf("getElementContent2:\n");
    char* element2 = getElementContent2(&httpPutBody, "ns0:priority");
    printf("getElementContent2: returned %s\n", element2);
    free(element2);


    /*
    httpPutBody.size = strlen(httpPutBody.data);
    httpPutBody.bytesCopied = 0;
    httpPutBody.bytesRemaining = httpPutBody.size;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;
    //printf("requestBody:\n\n%s\n", requestBody);
    httpPut(NULL, url, &httpPutBody, &httpResponseBody, &httpResponseHeaders);
    */
    free(httpPutBody.data);
    //free(httpResponseBody.data);
    //free(httpResponseHeaders.data);
}

void createAndActivateEVC(char* url, unsigned short ivid, connectionTp AEndTp, connectionTp ZEndTp)
{
    MemoryStruct httpPostBody;
    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    httpPostBody.data = createRequestBody2(ivid, AEndTp, ZEndTp);
    httpPostBody.size = strlen(httpPostBody.data);
    httpPostBody.bytesCopied = 0;
    httpPostBody.bytesRemaining = httpPostBody.size;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;
    //printf("requestBody:\n\n%s\n", requestBody);
    httpPost(NULL, url, &httpPostBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPostBody.data);
    free(httpResponseBody.data);
    free(httpResponseHeaders.data);
    /*
    char* requestBody;
    requestBody = createRequestBody2(ivid, AEndTp, ZEndTp);
    printf("requestBody:\n\n%s\n", requestBody);
    free(requestBody);
    */
}

void deleteEVC(char* url)
{

    return;
}

int main(void)
{
    short pcpToTcMappingTable[MAX_PCP] = {0,0,0,3,0,5,0,7};
    connectionTp AEndTp;
    AEndTp.parentTp = "/~ioam/~me/iNX8000/000000000001/~ptp/eth/shelf/1/slot/8/port/1";
    AEndTp.ivid = 251;
    AEndTp.portTpRoleState = "PTPR_FD_EDGE";
    connectionTp ZEndTp;
    ZEndTp.parentTp = "/~ioam/~me/iNX8000/000000000002/~ptp/eth/shelf/1/slot/8/port/1";
    ZEndTp.ivid = 251;
    ZEndTp.portTpRoleState = "PTPR_FD_EDGE";

    configurePCPtoTCMappings("http://84.203.208.121:8080/~ioam/~me/iNX8000/000000000001/~ptp/eth/shelf/1/slot/8/port/1", pcpToTcMappingTable, MAX_PCP);
    //sleep(2);
    //configurePCPtoTCMappings("http://84.203.208.121:8080/~ioam/~me/iNX8000/000000000002/~ptp/eth/shelf/1/slot/8/port/1", pcpToTcMappingTable, MAX_PCP);
    //sleep(2);
    //createAndActivateEVC("http://84.203.208.121:8080/~ioam/~me/iNX8000/000000000001/~ptp/eth/shelf/1/slot/8/port/1/~nsp/ecsV1_0/~evpl", 251, AEndTp, ZEndTp);
    return (0);

}

    //cmeGatewayIPAddress = 84.203.208.121
    //meName = iNX8000/000000000001
    //tpPath = shelf/1/slot/8
    //portId = 1
