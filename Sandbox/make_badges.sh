#!/bin/bash
cd `dirname $0`
root_id=$1
shift
artifacts_dir=$1
shift
excludes=" $* "
badges_dir=`./get_gdrive_id.sh $root_id badges` || ( echo "error getting badges directory"; exit 1 )

if [ -x badges_out ]; then
  rm -rf badges_out
fi
mkdir badges_out

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
    curl -o badges_out/${suite}.svg https://img.shields.io/static/v1?label=${suite}\&message=Pass\&color=brightgreen || ( echo "error downloading badge"; exit 1 )
  else
    curl -o badges_out/${suite}.svg https://img.shields.io/static/v1?label=${suite}\&message=Fail\&color=red || ( echo "error downloading badge"; exit 1 )
  fi
done

for badge in badges_out/*.svg; do
  badge_name=`basename $badge`
  badge_id=`$GDRIVE_CMD list --query "'${badges_dir}' in parents and name = '$badge_name'" | sed -n 2p | cut -d " " -f 1 -` || ( echo "error getting badge id"; exit 1 )
  if [[ "$badge_id" == "" ]]; then
    $GDRIVE_CMD upload --parent ${badges_dir} --mime "image/svg+xml" $badge
  else
    $GDRIVE_CMD update --mime "image/svg+xml" $badge_id $badge
  fi
done
