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

PACKET * resize_array(PACKET *old_packet, size_t new_len)
{
  PACKET *packet;
  packet = (PACKET *) realloc(old_packet, sizeof(PACKET) * new_len);
  if(packet == NULL){
    free(packet);
    return NULL;
  }

  return packet;
}

void free_packet(PACKET *packet)
{
  free(packet->data);
  free(packet);
  packet = NULL;
}
