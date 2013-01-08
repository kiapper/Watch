#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

void InitialiazeFrameBuffer(void);
void WriteBufferHandler(tHostMsg* pMsg);
void UpdateDisplayHandler(tHostMsg* pMsg);

#endif /* FRAME_BUFFER_H */
