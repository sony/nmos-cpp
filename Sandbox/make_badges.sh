#!/bin/bash
output_dir=$1
shift
artifacts_dir=$1
shift
excludes=" $* "

cd $output_dir

builds=()
for dir in ${artifacts_dir}/*_badges; do
  build=`basename $dir`
  build=${build%_badges}
  if [[ ! "$excludes" =~ .*\ $build\ .* ]]; then
    builds+=( $build )
  fi
done

for file in ${artifacts_dir}/${builds[0]}_badges/*.txt; do
  suite=`basename $file`
  suite="${suite%.*}"
  pass=true
  for build in ${builds[@]}; do
    if ! grep "Pass" ${artifacts_dir}/${build}_badges/${suite}.txt > /dev/null; then
      pass=false
      break
    fi
  done
  echo "$suite passed: $pass"
  if $pass; then
    curl -o ${suite}.svg https://img.shields.io/static/v1?label=${suite}\&message=Pass\&color=brightgreen || ( echo "error downloading badge"; exit 1 )
  else
    curl -o ${suite}.svg https://img.shields.io/static/v1?label=${suite}\&message=Fail\&color=red || ( echo "error downloading badge"; exit 1 )
  fi
done
