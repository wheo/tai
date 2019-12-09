CC=g++
SDK_PATH=../sdk/include
#CFLAGS=-Wno-multichar -I $(SDK_PATH) -fno-rtti #release
CFLAGS=-Wno-multichar -Wall -g -D_DEBUG -D__STDC_CONSTANT_MACROS -I $(SDK_PATH) -fno-rtti -I../tinyxml -I../ffmpeg/include -I../libjpeg/include
LDFLAGS= -lrt -lbz2 -lz -lm -ldl -lpthread -L../tinyxml -ltinyxml -L/usr/local/lib -ljpeg -L../ffmpeg/lib -lswscale -lavformat -lavcodec -lavutil -lswresample

.PHONY: tai clean install

tai: main.cpp global.cpp commc_apc.cpp commc_client.cpp comms.cpp queue.cpp decunit.cpp decklink_input.cpp renderer.cpp decklink.cpp scheduling.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp
	$(CC) -g -o tai global.cpp main.cpp queue.cpp commc_apc.cpp commc_client.cpp comms.cpp decunit.cpp decklink_input.cpp renderer.cpp decklink.cpp scheduling.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp $(CFLAGS) $(LDFLAGS)

clean:
	rm -f /opt/tai/tai
	rm -f tai

install:
	cp ./tai /opt/tai/.

