#include <stdio.h>
#include <server.h>

PACKET * create_packet()
{
  PACKET *packet;
  packet = (PACKET *) calloc(1, sizeof(PACKET));
  if(packet == NULL){
    free(packet);
    return NULL;
  }
  return packet;
}

bool resize_array(PACKET **oldPtr, size_t newLen)
{
  PACKET *newPtr = NULL;
  if(newLen < 0)
    return false;
  else
  if(newLen == 0){
    //free(*oldPtrdata);
    free(*oldPtr);
    *oldPtr = NULL;
    return true;
  }
  else{
    newPtr = (PACKET *) realloc(*oldPtr, newLen * sizeof(PACKET));
    if(newPtr == NULL){
      free(newPtr);
      return false;
    }
  }
  *oldPtr = newPtr;
  return true;
}

void free_packet(PACKET *packet)
{
  free(packet->data);
  free(packet);
  packet = NULL;
}
