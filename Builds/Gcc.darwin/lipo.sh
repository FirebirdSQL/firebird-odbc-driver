#!/bin/sh

lipo release_i386/libOdbcFB.dylib release_x86_64/libOdbcFB.dylib -output libOdbcFB.dylib -create
