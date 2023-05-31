#!/bin/bash

echo "`grep "Seconds passed between the first and the last read:" server123.log | tail -n 1 | cut -d ' ' -f10` - `grep "Total delay is" client123.log | sort -nrk4,4 | head -1 | cut -d ' ' -f4`" | bc