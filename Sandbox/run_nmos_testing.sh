#!/bin/bash

run_python=$1
shift
node_command=$1
shift
registry_command=$1
shift
results_dir=$1
shift
badges_dir=$1
shift
host_ip=$1
shift
build_prefix=$1
shift

cd `dirname $0`
self_dir=`pwd`
cd -

expected_disabled_BCP_003_01=0
# test_02, test_02_01, test_12
expected_disabled_IS_04_01=3
expected_disabled_IS_04_03=0
expected_disabled_IS_05_01=0
expected_disabled_IS_05_02=0
expected_disabled_IS_07_01=0
expected_disabled_IS_07_02=0
expected_disabled_IS_08_01=0
expected_disabled_IS_08_02=0
# test_02, test_02_01
expected_disabled_IS_09_02=2
expected_disabled_IS_04_02=0
expected_disabled_IS_09_01=0

config_secure=`${run_python} -c $'from nmostesting import Config\nprint(Config.ENABLE_HTTPS)'` || (echo "error running python"; exit 1)
config_auth=`${run_python} -c $'from nmostesting import Config\nprint(Config.ENABLE_AUTH)'` || (echo "error running python"; exit 1)

if [[ "${config_secure}" == "True" ]]; then
  secure=true
  echo "Running TLS tests"
  host=nmos-api.local
  common_params=",\"client_secure\":true,\
  \"server_secure\":true,\
  \"ca_certificate_file\":\"test_data/BCP00301/ca/certs/ca.cert.pem\",\
  \"server_certificates\":\
  [{\
    \"key_algorithm\":\"ECDSA\",\
    \"private_key_file\":\"test_data/BCP00301/ca/intermediate/private/ecdsa.api.testsuite.nmos.tv.key.pem\",\
    \"certificate_chain_file\":\"test_data/BCP00301/ca/intermediate/certs/ecdsa.api.testsuite.nmos.tv.cert.chain.pem\"\
  },\
  {\
    \"key_algorithm\":\"RSA\",\
    \"private_key_file\":\"test_data/BCP00301/ca/intermediate/private/rsa.api.testsuite.nmos.tv.key.pem\",\
    \"certificate_chain_file\":\"test_data/BCP00301/ca/intermediate/certs/rsa.api.testsuite.nmos.tv.cert.chain.pem\"\
  }],\
  \"dh_param_file\":\"test_data/BCP00301/ca/intermediate/private/dhparam.pem\",\
  \"host_name\":\"${host}\",\
  \"host_address\":\"${host_ip}\"\
  "
  registry_url=https://${host}:8088
else
  secure=false
  echo "Running non-TLS tests"
  host=${host_ip}
  common_params=
  registry_url=http://${host}:8088
fi

if [[ "${config_auth}" == "True" ]]; then
  echo "Running Auth tests"
  auth=true
else
  echo "Running non-Auth tests"
  auth=false
  (( expected_disabled_IS_04_01+=6 ))
  (( expected_disabled_IS_04_03+=6 ))
  (( expected_disabled_IS_05_01+=6 ))
  (( expected_disabled_IS_05_02+=12 ))
  (( expected_disabled_IS_07_01+=6 ))
  (( expected_disabled_IS_07_02+=18 ))
  (( expected_disabled_IS_08_01+=6 ))
  (( expected_disabled_IS_08_02+=12 ))
  (( expected_disabled_IS_04_02+=14 ))
  (( expected_disabled_IS_09_01+=6 ))
fi

common_params="${common_params} ,\"domain\":\"local\",\"logging_level\":-40"

"${node_command}" "{\"how_many\":6,\"http_port\":1080 ${common_params}}" > ${results_dir}/nodeoutput 2>&1 &
NODE_PID=$!

function do_run_test() {
  suite=$1
  echo "Running $suite"
  shift
  max_disabled_tests=$1
  shift
  max_disabled_file="${self_dir}/nmos-testing-options/${suite}_max_disabled.txt"
  if [[ -f "$max_disabled_file" ]]; then
    max_disabled_tests=`cat "$max_disabled_file"`
    echo "Max disabled tests overridden: $max_disabled_tests"
  fi
  options_file="${self_dir}/nmos-testing-options/${suite}.txt"
  suite_options=
  if [[ -f "$options_file" ]]; then
    suite_options=`cat "$options_file"`
    echo "Using additional options: $suite_options"
  fi
  output_file=${results_dir}/${build_prefix}${suite}.json
  result=$(${run_python} nmos-test.py suite ${suite} --selection all "$@" --output "${output_file}" $suite_options >> ${results_dir}/testoutput 2>&1; echo $?)
  if [ ! -e ${output_file} ]; then
    echo "No output produced"
    result=2
  else
    disabled_tests=`grep -c '"Test Disabled"' ${output_file}`
    if [[ $disabled_tests -gt $max_disabled_tests ]]; then
      echo "$disabled_tests tests disabled, expected max $max_disabled_tests"
      result=2
    elif [[ $disabled_tests -gt 0 ]]; then
      echo "$disabled_tests tests disabled"
    fi
  fi
  case $result in
  [0-1])  echo "Pass" | tee ${badges_dir}/${suite}.txt ;;
  *)      echo "Fail" | tee ${badges_dir}/${suite}.txt ;;
  esac
}

if $secure; then
  do_run_test BCP-003-01 $expected_disabled_BCP_003_01 --host "${host}" --port 1080 --version v1.0
fi

do_run_test IS-04-01 $expected_disabled_IS_04_01 --host "${host}" --port 1080 --version v1.3

do_run_test IS-04-03 $expected_disabled_IS_04_03 --host "${host}" --port 1080 --version v1.3

do_run_test IS-05-01 $expected_disabled_IS_05_01 --host "${host}" --port 1080 --version v1.1

do_run_test IS-05-02 $expected_disabled_IS_05_02 --host "${host}" "${host}" --port 1080 1080 --version v1.3 v1.1

do_run_test IS-07-01 $expected_disabled_IS_07_01 --host "${host}" --port 1080 --version v1.0

do_run_test IS-07-02 $expected_disabled_IS_07_02 --host "${host}" "${host}" "${host}" --port 1080 1080 1080 --version v1.3 v1.1 v1.0

do_run_test IS-08-01 $expected_disabled_IS_08_01 --host "${host}" --port 1080 --version v1.0 --selector null

do_run_test IS-08-02 $expected_disabled_IS_08_02 --host "${host}" "${host}" --port 1080 1080 --version v1.3 v1.0 --selector null null

do_run_test IS-09-02 $expected_disabled_IS_09_02 --host "${host}" null --port 0 0 --version null v1.0

# Run Registry tests (leave Node running)
"${registry_command}" "{\"pri\":0,\"http_port\":8088 ${common_params}}" > ${results_dir}/registryoutput 2>&1 &
REGISTRY_PID=$!
# short delay to give the Registry a chance to start up and the Node a chance to register before running the Registry test suite
sleep 2
# add a persistent Query WebSocket API subscription before running the Registry test suite
curl --cacert test_data/BCP00301/ca/certs/ca.cert.pem "${registry_url}/x-nmos/query/v1.3/subscriptions" -H "Content-Type: application/json" -d "{\"max_update_rate_ms\": 100, \"resource_path\": \"/nodes\", \"params\": {\"label\": \"host1\"}, \"persist\": true, \"secure\": ${secure}}" || echo "failed to add subscription"

do_run_test IS-04-02 $expected_disabled_IS_04_02 --host "${host}" "${host}" --port 8088 8088 --version v1.3 v1.3

do_run_test IS-09-01 $expected_disabled_IS_09_01 --host "${host}" --port 8088 --version v1.0

# Stop Node and Registry
kill $NODE_PID || echo "node not running"
kill $REGISTRY_PID || echo "registry not running"

