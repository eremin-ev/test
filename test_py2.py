#!/usr/bin/env python
# coding: utf-8

import json

def xstr(s):
    if s is None:
        return ''
    return str(s)

if __name__ == "__main__":
    iname = 'py-o.json'
    with open(iname, 'r') as f:
        data = json.load(f)

    s = json.dumps(data, indent=4, sort_keys=False)
    #print(s)

    #print(dir(data))

    #for key, value in data.items():
    #    print(key, type(value))

    if 'formats' in data:
        #print(type(data['formats']))
        #x = [name for name, age in data.items() if age == search_age]
        for f in data['formats']:
            fps = xstr(f['fps'])
            res = f'{f["height"]}x{f["width"]}'
            print(f'{f["format_note"]:>5}'
                  f'{f["format_id"]:>5} {fps:>5}',
                  f'{f["acodec"]:>10} {f["vcodec"]:>12} {f["tbr"]:9}',
                  f'{res:>10}',
                  f'\t{f["format"]}')
