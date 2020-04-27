#!/bin/bash
cd `dirname $0`
root_id=$1
shift
excludes=" $* "
build_dir=`./get_gdrive_id.sh $root_id build` || ( echo "error getting build directory"; exit 1 )
badges_dir=`./get_gdrive_id.sh $root_id badges` || ( echo "error getting badges directory"; exit 1 )

if [ -x badges_build ]; then
  rm -rf badges_build
fi
mkdir badges_build
$GDRIVE_PATH download $build_dir --recursive --path badges_build || ( echo "error downloading build results"; exit 1 )

if [ -x badges_out ]; then
  rm -rf badges_out
fi
mkdir badges_out

builds=()
for dir in badges_build/build/*; do
  build=`basename $dir`
  if [[ ! "$excludes" =~ .*\ $build\ .* ]]; then
    builds+=( $build )
  fi
done

for file in badges_build/build/${builds[0]}/*.json; do
  suite=`basename $file`
  suite="${suite%.*}"
  pass=true
  for build in ${builds[@]}; do
    if ! grep "\"message\":\"Pass\"" badges_build/build/$build/${suite}.json > /dev/null; then
      pass=false
      break
    fi
  done
  echo "$suite passed: $pass"
  if $pass; then
    curl  -o badges_out/${suite}.svg https://img.shields.io/static/v1?label=${suite}\&message=Pass\&color=brightgreen || ( echo "error downloading badge"; exit 1 )
  else
    curl  -o badges_out/${suite}.svg https://img.shields.io/static/v1?label=${suite}\&message=Fail\&color=red || ( echo "error downloading badge"; exit 1 )
  fi
done

$GDRIVE_PATH sync upload --keep-local --delete-extraneous badges_out $badges_dir || ( echo "error uploading badges"; exit 1 )
