# ERS RAW Parser

Library to parse ERS RAW SAR Datasets

## Description

This is an auxiliary library that is able to read ERS RAW data format and extract various parameters needed for the image formation process. The library does not extract all of the parameters avaible, just the ones I requires when doing the process.

Most of the units of the parameters are the same as the original dataset. But anyhow, most of them (I hope) are documented in the header file.

## Build

A simple:

```shell
$ make
$ make install
```

The default install path is `/usr/lib/` and is not parameterized.

## Usage

A usage example can be found in: `./tools/`

```shell
$ ./bin/ers_raw_parser_tool --ldr=<ldr file path>  --raw=<raw file path>
```

But basically, you just have to allocate and initialize a context with the path to the data, and then you have functions to extract both the parameters from the ldr file and the raw data. Then remember to free everything, both the data patches and the context.

The data is read in patches, each of them of sizes equivalent to the azimuth fft lines.
