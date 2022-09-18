#!/usr/bin/env python3
import os
import struct
import json
import sys

if len(sys.argv) != 3:
    print(f'Usage: {sys.argv[0]} <login packet json> <binary output directory>')
    exit(-1)

login_packet = json.load(open(sys.argv[1]))
codec = login_packet['dimensionCodec']

dimension_list = codec['value']['minecraft:dimension_type']['value']['value']['value']['value']


########################################################################################################################
# NBT Spec
########################################################################################################################


TAG_End = b'\x00'
TAG_Byte = b'\x01'
TAG_Short = b'\x02'
TAG_Int = b'\x03'
TAG_Long = b'\x04'
TAG_Float = b'\x05'
TAG_Double = b'\x06'
TAG_Byte_Array = b'\x07'
TAG_String = b'\x08'
TAG_List = b'\x09'
TAG_Compound = b'\x0a'
TAG_Int_Array = b'\x0b'
TAG_Long_Array = b'\x0c'


TAG_TYPES = {
    'byte': TAG_Byte,
    'short': TAG_Short,
    'int': TAG_Int,
    'long': TAG_Long,
    'float': TAG_Float,
    'double': TAG_Double,
    'string': TAG_String,
    'list': TAG_List,
    'compound': TAG_Compound,
}


########################################################################################################################
# Functions to encode
########################################################################################################################


def encode_raw_string(f, s):
    data = bytes(s, 'ascii')
    f.write(struct.pack(">H", len(data)))
    f.write(data)


def encode_tag(f, typ, name, value):
    if name is not None:
        f.write(TAG_TYPES[typ])
        encode_raw_string(f, name)

    if typ == 'compound':
        for item_name in value:
            item_typ = value[item_name]['type']
            item_val = value[item_name]['value']
            encode_tag(f, item_typ, item_name, item_val)
        f.write(TAG_End)
    elif typ == 'list':
        element_type = value['type']
        elements = value['value']
        f.write(TAG_TYPES[element_type])
        f.write(struct.pack(">I", len(elements)))
        for element in elements:
            encode_tag(f, element_type, None, element)
    elif typ == 'string':
        encode_raw_string(f, value)
    elif typ == 'byte':
        f.write(struct.pack(">B", value))
    elif typ == 'int':
        f.write(struct.pack(">I", value))
    elif typ == 'long':
        value = (value[0] << 32) | value[1]
        f.write(struct.pack(">Q", value))
    elif typ == 'float':
        f.write(struct.pack(">f", value))
    elif typ == 'double':
        f.write(struct.pack(">d", value))
    else:
        assert False, f"Invalid type {typ}"


####################################################################################################################
# The codec itself
####################################################################################################################

with open(os.path.join(sys.argv[2], 'dimension_codec.nbt'), 'wb') as f:
    encode_tag(f, codec['type'], codec['name'], codec['value'])

####################################################################################################################
# The dimensions, for reference
####################################################################################################################

for dim in dimension_list:
    name = dim['name']['value']
    value = dim['element']
    with open(os.path.join(sys.argv[2], name.split(':')[1] + '.nbt'), 'wb') as f:
        encode_tag(f, value['type'], '', value['value'])
