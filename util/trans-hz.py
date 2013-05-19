#!/usr/bin/env python2
# -*- coding: utf-8 -*-

tables = ["数","据","集","中","器","转","发","热","总","表","检","测","网","络","服","务","无","信","号","强","远","程","连","接","成","功","失","败","状","态","已","未","阀","控","量","报","警","息","离","线","故","障","供","回","水","累","计","温","度","地","址","时","间","结","算","冷","流","瞬","差","△","℃","欢","迎","使","用","仪","注","册","没","有","请","后","按","键","等","待","正","在","设","置","读","取","编","号","户","设","备","端","口","确","认","删","除","取","消","第","显","示","退","出","输","入","密","码","模","块","通","任","意","返","回","‰","㎥","常","刷","卡","到","授","权","磁","盘","份","完","拔","启","当","前"]

def convert(src, dst):
    isstr = False;
    strbuf = ''
    with open(src, 'r') as fin, open(dst, 'w') as fout:
        for line in fin:
            for c in line:
                if c == '"':
                    if isstr == False:
                        isstr = True
                        strbuf = ''
                    else:
                        isstr = False
                        buf = ''
                        for ch in strbuf.decode('utf-8'):
                            if ord(ch) > 255 and ch.encode('utf-8') not in tables:
                                print 'Missing', ch, 'in tables'
                            elif ch.encode('utf-8') not in tables:
                                buf += ch
                            else:
                                idx = tables.index(ch.encode('utf-8'))
                                if idx == 8:
                                    buf += '\\b'
                                elif idx == 9:
                                    buf += '\\t'
                                else:
                                    buf += '\%d' % idx
                        fout.write('"%s"' % buf)
                else:
                    if isstr == False:
                        fout.write(c)
                    else:
                        strbuf+=c
        pass

if __name__ == '__main__':
    import sys
    import os.path
    if len(sys.argv) < 3 or not os.path.exists(sys.argv[1]):
        print 'Usage: trans-hz.py source target'
    else:
        convert(sys.argv[1], sys.argv[2])
