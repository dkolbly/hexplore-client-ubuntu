PNG_CONFIG=libpng-config

OFILES=curve.o hex.o pick.o regionpicker.o picture.o SimplexNoise.o \
	region.o ico.o misc.o randompixel.o ssl.o


sslserver: libhexcom.a ssltest.cpp
	g++ $(CFLAGS) ssltest.cpp -L. -lhexcom -lpthread -lssl -lcrypto

libhexcom.a: $(OFILES)
	ar cru libhexcom.a $(OFILES) 
	ranlib libhexcom.a

# On ubuntu 13.10 we get warnings from libpng12 (png.h)
# see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=676157

CFLAGS=-g -Wall -std=c++11 `$(PNG_CONFIG) --cflags`

%.o: %.cpp
	g++ $(CFLAGS) -MD -c $< -o $@

clean::
	rm -f $(OFILES) *.d libhexcom.a

-include *.d

