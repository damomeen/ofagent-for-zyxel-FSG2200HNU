#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>

#define MAX_PCP 8

typedef struct {
    char *data;
    size_t size;
    int bytesCopied;
    int bytesRemaining;
} MemoryStruct;

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
	char* parentTp[1000];
	unsigned short ivid;
	char* portTpRoleState;
	eLineTrafficMappingTable bandwidthProfile;

} connectionTp;

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
            printf("******Wczytany source: \n%s\n", source);
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
    return NULL;
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
    return NULL;
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

void configurePCPtoTCMappings(char* url, unsigned short pcpToTcMappingTable[], unsigned short tableSize){
    MemoryStruct httpPutBody;
    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    httpPutBody.data = createRequestBody1(pcpToTcMappingTable, tableSize);
    httpPutBody.size = strlen(httpPutBody.data);
    httpPutBody.bytesCopied = 0;
    httpPutBody.bytesRemaining = httpPutBody.size;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;
    //printf("requestBody:\n\n%s\n", requestBody);
    httpPut(NULL, url, &httpPutBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPutBody.data);
    free(httpResponseBody.data);
    free(httpResponseHeaders.data);
}

char* _createAndActivateEVC(char* url, unsigned short ivid, connectionTp AEndTp, connectionTp ZEndTp)
{
    MemoryStruct httpPostBody;
    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    char* evcID;

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
    //TODO: CHECK IF SUCCESS
    evcID = getElementContent(&httpResponseBody, "ns0:fdfrRef");

    free(httpPostBody.data);
    free(httpResponseBody.data);
    free(httpResponseHeaders.data);
    return evcID;
    /*
    char* requestBody;
    requestBody = createRequestBody2(ivid, AEndTp, ZEndTp);
    printf("requestBody:\n\n%s\n", requestBody);
    free(requestBody);
    */
}

void _deleteEVC(char* url)
{
    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;
    httpDelete(NULL, url, &httpResponseBody, &httpResponseHeaders);
    free(httpResponseBody.data);
    free(httpResponseHeaders.data);
    return;
}

struct intune_port_url {
    short ofPortNumber;
    char *cmeGatewayIPAddress;
    char *meName;
    char *tpPath;
    short portId;
};

struct evc_params {
    uint16_t vid;
    uint16_t in_port;
    uint16_t out_port;
    short flow_counter;
};

struct intune_properties {
    uint32_t wildcards;
    //struct dev_action* supported_actions;
    int supported_actions_len;
    struct intune_port_url of_to_intune_portmap[10];
    short int pcpToTcMappingTable[10][8];
    struct evc_params created_evcs[1000];
    short evc_counter;
    char *cmeGatewayIPAddress;
};

struct intune_properties intune_props;

char* createAndActivateEVC(int ivid, int in_port, int out_port)
{
    //dzia³ajace zestawienie EVPLa na Intune
    intune_props.cmeGatewayIPAddress = "84.203.225.27";
    intune_props.pcpToTcMappingTable[1][0] = 0;
    intune_props.pcpToTcMappingTable[1][1] = 0;
    intune_props.pcpToTcMappingTable[1][2] = 0;
    intune_props.pcpToTcMappingTable[1][3] = 3;
    intune_props.pcpToTcMappingTable[1][4] = 0;
    intune_props.pcpToTcMappingTable[1][5] = 5;
    intune_props.pcpToTcMappingTable[1][6] = 0;
    intune_props.pcpToTcMappingTable[1][7] = 7;

    intune_props.pcpToTcMappingTable[2][0] = 0;
    intune_props.pcpToTcMappingTable[2][1] = 0;
    intune_props.pcpToTcMappingTable[2][2] = 0;
    intune_props.pcpToTcMappingTable[2][3] = 3;
    intune_props.pcpToTcMappingTable[2][4] = 0;
    intune_props.pcpToTcMappingTable[2][5] = 5;
    intune_props.pcpToTcMappingTable[2][6] = 0;
    intune_props.pcpToTcMappingTable[2][7] = 7;

    intune_props.of_to_intune_portmap[1].ofPortNumber = 1;
    intune_props.of_to_intune_portmap[1].cmeGatewayIPAddress = intune_props.cmeGatewayIPAddress;
    intune_props.of_to_intune_portmap[1].meName = "iNX8000/000000000001";
    intune_props.of_to_intune_portmap[1].tpPath = "shelf/1/slot/8";
    intune_props.of_to_intune_portmap[1].portId = 1;

    intune_props.of_to_intune_portmap[2].ofPortNumber = 2;
    intune_props.of_to_intune_portmap[2].cmeGatewayIPAddress = intune_props.cmeGatewayIPAddress;
    intune_props.of_to_intune_portmap[2].meName = "iNX8000/000000000002";
    intune_props.of_to_intune_portmap[2].tpPath = "shelf/1/slot/8";
    intune_props.of_to_intune_portmap[2].portId = 1;

    char *evcID;
    //short pcpToTcMappingTable[MAX_PCP] = {0,0,0,3,0,5,0,7};
    //char *cmeGatewayIPAddress = "84.203.225.27";
    //char *meName1 = "iNX8000/000000000001";
    //char *meName2 = "iNX8000/000000000002";
    //char *tpPath = "shelf/1/slot/8";
    //short portId = 1;
    char url[1000];

    connectionTp AEndTp;
    //AEndTp.parentTp = "/~ioam/~me/iNX8000/000000000001/~ptp/eth/shelf/1/slot/8/port/1";
    snprintf(url, sizeof url, "/~ioam/~me/%s/~ptp/eth/%s/port/%d", intune_props.of_to_intune_portmap[in_port].meName, intune_props.of_to_intune_portmap[in_port].tpPath, intune_props.of_to_intune_portmap[in_port].portId);
    strcpy(&AEndTp.parentTp, &url);
    AEndTp.ivid = ivid;
    AEndTp.portTpRoleState = "PTPR_FD_EDGE";
    printf("\nurl po AEndTp: %s\n", url);

    connectionTp ZEndTp;
    //ZEndTp.parentTp = "/~ioam/~me/iNX8000/000000000002/~ptp/eth/shelf/1/slot/8/port/1";
    memset(url, 0, 1000);
    snprintf(url, sizeof url, "/~ioam/~me/%s/~ptp/eth/%s/port/%d", intune_props.of_to_intune_portmap[out_port].meName, intune_props.of_to_intune_portmap[out_port].tpPath, intune_props.of_to_intune_portmap[out_port].portId);
    strcpy(&ZEndTp.parentTp, &url);
    ZEndTp.ivid = ivid;
    ZEndTp.portTpRoleState = "PTPR_FD_EDGE";
    printf("\nurl po ZEndTp: %s\n", url);

    memset(url, 0, 1000);
    snprintf(url, sizeof url, "http://%s:8080/~ioam/~me/%s/~ptp/eth/%s/port/%d", intune_props.of_to_intune_portmap[in_port].cmeGatewayIPAddress, intune_props.of_to_intune_portmap[in_port].meName, intune_props.of_to_intune_portmap[in_port].tpPath, intune_props.of_to_intune_portmap[in_port].portId);
    printf("\nurl przed step1: %s\n", url);
    configurePCPtoTCMappings(url, intune_props.pcpToTcMappingTable[in_port], 8);
    sleep(1);

    memset(url, 0, 1000);
    snprintf(url, sizeof url, "http://%s:8080/~ioam/~me/%s/~ptp/eth/%s/port/%d", intune_props.of_to_intune_portmap[out_port].cmeGatewayIPAddress, intune_props.of_to_intune_portmap[out_port].meName, intune_props.of_to_intune_portmap[out_port].tpPath, intune_props.of_to_intune_portmap[out_port].portId);
    printf("\nurl przed step2: %s\n", url);
    configurePCPtoTCMappings(url, intune_props.pcpToTcMappingTable[out_port], 8);
    sleep(1);

    memset(url, 0, 1000);
    snprintf(url, sizeof url, "http://%s:8080/~ioam/~me/%s/~ptp/eth/%s/port/%d/~nsp/ecsV1_0/~evpl", intune_props.of_to_intune_portmap[in_port].cmeGatewayIPAddress, intune_props.of_to_intune_portmap[in_port].meName, intune_props.of_to_intune_portmap[in_port].tpPath, intune_props.of_to_intune_portmap[in_port].portId);
    evcID = _createAndActivateEVC(url, ivid, AEndTp, ZEndTp);
    printf("\nevcID: %s\n", evcID);
    sleep(1);
    return evcID;
}

void deleteEVC(char* evcID)
{
    char url[1000];
    snprintf(url, sizeof url, "http://%s:8080%s", intune_props.cmeGatewayIPAddress, evcID);
    printf("\ndeleteUrl: %s\n", url);
    _deleteEVC(url);
    free(evcID);
    return;
}

int main(int argc, int **argv)
{
    char* evcID1;
    char* evcID2;
    char* evcID3;
    evcID1 = createAndActivateEVC(100, 1, 2);
    evcID2 = createAndActivateEVC(200, 2, 1);
    evcID3 = createAndActivateEVC(300, 2, 1);
    deleteEVC(evcID1);
    deleteEVC(evcID2);
    deleteEVC(evcID3);
}

    //cmeGatewayIPAddress = 84.203.208.121
    //meName = iNX8000/000000000001
    //tpPath = shelf/1/slot/8
    //portId = 1

    /*
   //curl_global_init(CURL_GLOBAL_ALL);                                                //init the curl session

    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    //STEP 1
    char *urlString1 = "http://84.203.208.121:8080/~ioam/~me/iNX8000/000000000001/~ptp/eth/shelf/1/slot/8/port/1";
    MemoryStruct httpPutBody;
    httpPutBody.data = readFileToBuffer(argv[1]);
    httpPutBody.size = strlen(httpPutBody.data);      //nie potrzeba nulla na koncu dawac
    httpPutBody.bytesCopied = 0;
    httpPutBody.bytesRemaining = httpPutBody.size;
    httpPut(NULL, urlString1, &httpPutBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPutBody.data);

    printf("Wczytane responseBody ze STEP1: \n%s\n", httpResponseBody.data);
    printf("Wczytane responseHeaders ze STEP1: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    sleep(5);

    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    //STEP 2
    char *urlString2 = "http://84.203.208.121:8080/~ioam/~me/iNX8000/000000000002/~ptp/eth/shelf/1/slot/8/port/1";
    httpPutBody.data = readFileToBuffer(argv[2]);
    httpPutBody.size = strlen(httpPutBody.data);      //nie potrzeba nulla na koncu dawac
    httpPutBody.bytesCopied = 0;
    httpPutBody.bytesRemaining = httpPutBody.size;
    httpPut(NULL, urlString2, &httpPutBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPutBody.data);

    printf("Wczytane responseBody ze STEP2: \n%s\n", httpResponseBody.data);
    printf("Wczytane responseHeaders ze STEP2: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    sleep(5);

    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    //STEP 3
    char *urlString3 = "http://84.203.208.121:8080/~ioam/~me/iNX8000/000000000001/~ptp/eth/shelf/1/slot/8/port/1/~nsp/ecsV1_0/~evpl";
    MemoryStruct httpPostBody;
    httpPostBody.data = readFileToBuffer(argv[3]);
    httpPostBody.size = strlen(httpPostBody.data) - 1;      //nie potrzeba nulla na koncu dawac
    httpPostBody.bytesCopied = 0;
    httpPostBody.bytesRemaining = httpPostBody.size;
    printf("Wczytane httpPostBody ze STEP3: \n%s\n", httpPostBody.data);
    httpPost(NULL, urlString3, &httpPostBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPostBody.data);

    printf("Wczytane responseBody ze STEP3: \n%s\n", httpResponseBody.data);
    printf("Wczytane responseHeaders ze STEP3: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    //curl_global_cleanup();

    /* MemoryStruct httpPutBody;
    httpPutBody.data = readFileToBuffer(argv[1]);
    httpPutBody.size = strlen(httpPutBody.data);      //nie potrzeba nulla na koncu dawac
    httpPutBody.bytesCopied = 0;
    httpPutBody.bytesRemaining = httpPutBody.size;

    MemoryStruct httpPostBody;
    httpPostBody.data = readFileToBuffer(argv[1]);
    httpPostBody.size = strlen(httpPostBody.data);      //nie potrzeba nulla na koncu dawac
    httpPostBody.bytesCopied = 0;
    httpPostBody.bytesRemaining = httpPostBody.size;

    MemoryStruct httpResponseBody;
    MemoryStruct httpResponseHeaders;
    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    httpPut(NULL, urlString, &httpPutBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPutBody.data);

    printf("Wczytane body z httpPut: \n%s\n", httpResponseBody.data);
    printf("Wczytane hedery z httpPut: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    sleep(3);

    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    httpPost(NULL, urlString, &httpPostBody, &httpResponseBody, &httpResponseHeaders);
    free(httpPostBody.data);

    printf("Wczytane body z httpPost: \n%s\n", httpResponseBody.data);
    printf("Wczytane hedery z httpPost: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    sleep(3);

    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    httpGet(NULL, urlString, &httpResponseBody, &httpResponseHeaders);

    printf("Wczytane body z geta: \n%s\n", httpResponseBody.data);
    printf("Wczytane hedery z geta: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data);

    sleep(3);

    httpResponseBody.data = malloc(1);
    httpResponseBody.size = 0;
    httpResponseHeaders.data = malloc(1);
    httpResponseHeaders.size = 0;

    httpDelete(NULL, urlString, &httpResponseBody, &httpResponseHeaders);

    printf("Wczytane body z delete: \n%s\n", httpResponseBody.data);
    printf("Wczytane hedery z delete: \n%s\n", httpResponseHeaders.data);

    if (httpResponseBody.data) free(httpResponseBody.data);
    if (httpResponseHeaders.data) free(httpResponseHeaders.data); */
