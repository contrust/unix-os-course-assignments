#!/bin/bash

echo "`grep "Seconds passed between the first and the last read:" /tmp/server123.log | tail -n 1 | cut -d ' ' -f10` - `grep "Total delay is" /tmp/client123.log | sort -nrk4,4 | head -1 | cut -d ' ' -f4`" | bc