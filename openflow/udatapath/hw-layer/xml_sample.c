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

#include <stdio.h>
#include <stdlib.h>
#include <libxml/parser.h>

#define MAX_PCP 8
unsigned short pcpToTcMappingTable[MAX_PCP] = {0,0,0,3,0,5,0,7};

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


char* createRequestBody(unsigned short pcpToTcMappingTable[], unsigned short tableSize){
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

void configurePCPtoTCMappings(char* url, unsigned short pcpToTcMappingTable[], unsigned short tableSize){
    char* requestBody;
    requestBody = createRequestBody(pcpToTcMappingTable, tableSize);
    printf("requestBody:\n\n%s\n", requestBody);
    free(requestBody);
}

int
main(void)
{
    configurePCPtoTCMappings(NULL, pcpToTcMappingTable, MAX_PCP);
    //xmlNodeSetContent(n, BAD_CAST "content");
    /*
     * xmlNewChild() creates a new node, which is "attached" as child node
     * of root_node node.
     */
    //xmlNewChild(root_node, NULL, BAD_CAST "node1", BAD_CAST "content of node 1");
    /*
     * xmlNewProp() creates attributes, which is "attached" to an node.
     * It returns xmlAttrPtr, which isn't used here.
     */


    /*
     * Dump the document to a buffer and print it
     * for demonstration purposes.
     */
    return (0);

}
