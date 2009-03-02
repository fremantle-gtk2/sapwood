#!/bin/sh

# NB#22173 memory saving hack, depends on modified gtk+
export GDK_DISABLE_XSHM=1
/usr/sbin/dsmetool -t /usr/lib/sapwood/sapwood-server
unset GDK_DISABLE_XSHM
