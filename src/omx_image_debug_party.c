/*
omx_image_debug_party.c - OMX debugging helper routines
This is based entirely on code from:
Copyright 2012, 2013 Dickon Hood <dickon@fluff.org>

https://github.com/dickontoo/omxtx/blob/master/omxtx.c

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

static OMX_VERSIONTYPE SpecificationVersion = {
        .s.nVersionMajor = 1,
        .s.nVersionMinor = 1,
        .s.nRevision     = 2,
        .s.nStep         = 0
};

#define MAKEME(y, x)        do {                                                \
                                y = calloc(1, sizeof(x));                \
                                y->nSize = sizeof(x);                        \
                                y->nVersion = SpecificationVersion;        \
                        } while (0)


#define OERR(cmd)        do {                                                \
                                OMX_ERRORTYPE oerr = cmd;                \
                                if (oerr != OMX_ErrorNone) {                \
                                        printf(#cmd                \
                                                " failed on line %d: %x\n", \
                                                __LINE__, oerr);        \
                                        exit(1);                        \
                                } else {                                \
                                        printf(#cmd                \
                                                " completed at %d.\n",        \
                                                __LINE__);                \
                                }                                        \
                        } while (0)

#define OERRq(cmd)        do {        oerr = cmd;                                \
                                if (oerr != OMX_ErrorNone) {                \
                                        fprintf(stderr, #cmd                \
                                                " failed: %x\n", oerr);        \
                                        exit(1);                        \
                                }                                        \
                        } while (0)

static void dumpport (OMX_HANDLETYPE handle, int port) {
  OMX_VIDEO_PORTDEFINITIONTYPE *viddef;
  OMX_PARAM_PORTDEFINITIONTYPE *portdef;

  MAKEME (portdef, OMX_PARAM_PORTDEFINITIONTYPE);
  portdef->nPortIndex = port;
  OERR (OMX_GetParameter (handle, OMX_IndexParamPortDefinition, portdef));

  printf ("Port %d is %s, %s\n", portdef->nPortIndex, (portdef->eDir == 0 ? "input" : "output"), (portdef->bEnabled == 0 ? "disabled" : "enabled"));
  printf ("Wants %d bufs, needs %d, size %d, enabled: %d, pop: %d, "
          "aligned %d\n", portdef->nBufferCountActual, portdef->nBufferCountMin, portdef->nBufferSize, portdef->bEnabled, portdef->bPopulated, portdef->nBufferAlignment);
  viddef = &portdef->format.video;

  switch (portdef->eDomain) {
    case OMX_PortDomainVideo:
      printf ("Video type is currently:\n"
              "\tMIME:\t\t%s\n"
              "\tNative:\t\t%p\n"
              "\tWidth:\t\t%d\n"
              "\tHeight:\t\t%d\n"
              "\tStride:\t\t%d\n"
              "\tSliceHeight:\t%d\n"
              "\tBitrate:\t%d\n"
              "\tFramerate:\t%d (%x); (%f)\n"
              "\tError hiding:\t%d\n"
              "\tCodec:\t\t%d\n"
              "\tColour:\t\t%d\n",
              viddef->cMIMEType, viddef->pNativeRender,
              viddef->nFrameWidth, viddef->nFrameHeight,
              viddef->nStride, viddef->nSliceHeight,
              viddef->nBitrate, viddef->xFramerate, viddef->xFramerate, ((float) viddef->xFramerate / (float) 65536), viddef->bFlagErrorConcealment, viddef->eCompressionFormat, viddef->eColorFormat);
      break;
    case OMX_PortDomainImage:
      printf ("Image type is currently:\n"
              "\tMIME:\t\t%s\n"
              "\tNative:\t\t%p\n"
              "\tWidth:\t\t%d\n"
              "\tHeight:\t\t%d\n"
              "\tStride:\t\t%d\n"
              "\tSliceHeight:\t%d\n"
              "\tError hiding:\t%d\n"
              "\tCodec:\t\t%d\n"
              "\tColour:\t\t%d\n",
              portdef->format.image.cMIMEType,
              portdef->format.image.pNativeRender,
              portdef->format.image.nFrameWidth,
              portdef->format.image.nFrameHeight,
              portdef->format.image.nStride,
              portdef->format.image.nSliceHeight, portdef->format.image.bFlagErrorConcealment, portdef->format.image.eCompressionFormat, portdef->format.image.eColorFormat);
      break;
      /* Feel free to add others. */
    default:
      break;
  }

  free (portdef);
}


#define flag_checkpoint(a) printf(a);
#define debug_printf(fmt, ...) printf(fmt, ##__VA_ARGS__)