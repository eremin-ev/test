#!/usr/bin/env python
# coding: utf-8

import json
import youtube_dl


class MyLogger(object):
    def __init__(self):
        self.verbose = 0

    def debug(self, msg):
        if (self.verbose):
            print(msg)

    def warning(self, msg):
        #if (self.verbose):
        print(msg)

    def error(self, msg):
        print(msg)


def my_hook1(d):
    if d['status'] == 'finished':
        print('Done downloading')
    elif d['status'] == 'downloading':
        speed = d['downloaded_bytes'] / d['elapsed']
        print(f'speed avg {speed:10}')


url = 'http://www.youtube.com/watch?v=BaW_jenozKc'

#'outtmpl': '%(id)s.%(ext)s'
# vcodec=vp9
options = {
    'format': 'bestvideo[height=720][vcodec^=avc1]',
    'outtmpl': 'v',
    'nopart': True,
    'continuedl': True,
	'logger': MyLogger(),
    'progress_hooks': [my_hook1],
}

if __name__ == "__main__":
    with youtube_dl.YoutubeDL(options) as ydl:
        result = ydl.extract_info(url, download=True)

    if 'entries' in result:
        # Can be a playlist or a list of videos
        video = result['entries'][0]
    else:
        # Just a video
        video = result

    print("type(result)", type(video))
    print("type(video)", type(video))

    #j = json.loads(result)

    oname = 'py-o2.json'
    with open(oname, 'w') as f:
        json.dump(result, f, indent=4, sort_keys=False)

    #video_url = video['url']
    #print(video_url)
