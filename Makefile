#/*
# * Copyright (c) 2016 Rosimildo DaSilva <rosimildo@gmail.com>
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# * MA 02110-1301, USA.
# *
# */
CROSS_COMPILE ?= /opt/linaro/gcc-linaro-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

CC = $(CROSS_COMPILE)gcc
CPP =$(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar

TARGET=videoenc
BUILDPATH=.

SRCDIRS:=. Camera watermark

CFLAGS =-Wall -O3 -ldl -pthread -std=c++11 -ggdb

INCLUDES:=$(foreach dir,$(SRCDIRS),-I$(dir)) -I. -I./libyuv/include

SRCCS=$(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
SRCPPS=$(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))

LIBOBJ=$(addprefix $(BUILDPATH)/, $(addsuffix .o, $(basename $(SRCCS)))) 
LIBOBJ+=$(addprefix $(BUILDPATH)/, $(addsuffix .o, $(basename $(SRCPPS)))) 

#LDFLAGS=  -lcdx_base -ldl -lm -lpthread -lvencoder -lMemAdapter -lVE -L/usr/local/lib/cedarx
LDFLAGS= -L./../sunxi-cedarx2/src -ldl -lm -lpthread -lcedar_vencoder -lcedar_common -lcedar_base -ggdb ./libyuv/libyuv.a

all: $(TARGET)

$(BUILDPATH)/%.o:%.c
	$(CC) $(CFLAGS) ${INCLUDES} -o $@ -c $<

$(BUILDPATH)/%.o:%.cpp
	$(CPP) $(CFLAGS) ${INCLUDES} -o $@ -c $<

$(TARGET):$(LIBOBJ)
	$(CPP) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(LIBOBJ)
