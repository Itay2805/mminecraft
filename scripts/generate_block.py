from ctypes import *
from datetime import datetime
import os.path as path
import urllib.request
import json

import stringcase

blocks = json.load(open("../minecraft-data/data/pc/1.16.2/blocks.json", 'r'))

block_by_name = {}
for block in blocks:
    block_by_name[block['name']] = block

# Block variables
for block in blocks:
    name = stringcase.pascalcase(block['name'])
    print(f"var {name} = &Block{{")
    print(f"\tId: {block['id']},")
    print(f"\tName: \"{block['name']}\",")
    print(f"\tMinStateId: {block['minStateId']},")
    print(f"\tMaxStateId: {block['maxStateId']},")
    print(f"\tDefaultStateId: {block['defaultState']},")
    print(f"\tSolid: {'false' if block['boundingBox'] == 'empty' else 'true'},")
    print(f"\tTransparent: {'true' if block['transparent'] else 'false'},")
    print(f"\tFilterLight: {block['filterLight']},")
    print(f"\tEmitLight: {block['emitLight']},")
    # TODO: item
    print("}")
print()

# Block by id lookup
print("var blocks = [...]*Block{")
for block in blocks:
    name = stringcase.pascalcase(block['name'])
    print(f"\t{block['id']}: {name},")
print("}")
print("")

# Block by item id
print("var blockByItemId = [...]*Block{")
for item in items:
    if item['name'] in block_by_name:
        name = stringcase.pascalcase(item['name'])
        print(f"\t{item['id']}: {name},")
print("}")
print("")