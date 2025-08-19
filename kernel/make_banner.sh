#!/bin/sh

# Re-creates the banner.h file for each compilation run

printf 'static const char banner[] = \n' > banner.h

gitrev=''
relver="$KOS_VERSION"

printf '"KallistiOS ' >> banner.h
printf "v$relver" >> banner.h
printf ' [' >> banner.h
printf "$KOS_ARCH" >> banner.h
printf '/' >> banner.h
printf "$KOS_SUBARCH" >> banner.h
printf ']' >> banner.h
printf '\\n"\n' >> banner.h
if [ -d "$KOS_BASE/.git" ]; then
    printf '"  Git revision: ' >> banner.h
    gitrev=`git describe --dirty --always`
    printf "$gitrev" >> banner.h
    printf '\\n"\n' >> banner.h
fi

printf '"  ' >> banner.h
tmp=`date`
printf "$tmp" >> banner.h
printf '\\n"\n' >> banner.h

printf '"  ' >> banner.h
tmp=`whoami`
printf "$tmp" >> banner.h
printf '@' >> banner.h

if [ `uname` = Linux ]; then
    tmp=`hostname -f`
else
    tmp=`hostname`
fi

printf "$tmp" >> banner.h

printf ':' >> banner.h
printf "$KOS_BASE" >> banner.h
printf '\\n"\n' >> banner.h

printf '"  ' >> banner.h
tmp=`$KOS_CC --version | head -n 1`
printf "$tmp" >> banner.h
printf '\\n"\n' >> banner.h
printf ';\n' >> banner.h

printf 'static const char kern_version[] = \n"' >> banner.h

if [ -z "$gitrev" ]; then
    printf "$relver" >> banner.h
else
    printf "${gitrev#?}" >> banner.h
fi

printf '";\n' >> banner.h

