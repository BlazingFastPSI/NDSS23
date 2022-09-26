#!/bin/bash
#To measure CPU time, remove all traffic shaping, RTT throttling etc.
(time ./ot 1 12345) & (time ./ot 2 12345)
