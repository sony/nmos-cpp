#!/bin/bash -x
parent_id=$1
path=$2
IFS='/'
read -ra paths <<< "$path"
IFS=' '
new_path=false

for path in ${paths[@]}; do
  if $new_path; then
    folder_id=""
  else
    folder_id=`$GDRIVE_CMD list --query "'${parent_id}' in parents and name = '$path'" | sed -n 2p | cut -d " " -f 1 -` || ( echo "error getting diretory"; exit 1 )
  fi
  if [ "$folder_id" == "" ]; then
    parent_id=`$GDRIVE_CMD mkdir -p $parent_id $path | cut -d' ' -f2` || ( echo "error making diretory"; exit 1 )
    new_path=true
  else
    parent_id=$folder_id
  fi
done
echo $parent_id
