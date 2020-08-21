#!/bin/bash

run_test=$1
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
bulid_prefix=$1
shift

"${node_command}" "{\"how_many\":6,\"http_port\":1080,\"domain\":\"local.\",\"logging_level\":-40}" > ${results_dir}/nodeoutput 2>&1 &
NODE_PID=$!

function do_run_test() {
  suite=$1
  shift
  case $(${run_test} suite ${suite} --selection all "$@" --output "${results_dir}/${bulid_prefix}${suite}.json" >> ${results_dir}/testoutput 2>&1; echo $?) in
  [0-1])  echo "Pass" > ${badges_dir}/${suite}.txt ;;
  *)      echo "Fail" > ${badges_dir}/${suite}.txt ;;
  esac
}

do_run_test IS-04-01 --host "${host_ip}" --port 1080 --version v1.3

do_run_test IS-04-03 --host "${host_ip}" --port 1080 --version v1.3

do_run_test IS-05-01 --host "${host_ip}" --port 1080 --version v1.1

do_run_test IS-05-02 --host "${host_ip}" "${host_ip}" --port 1080 1080 --version v1.3 v1.1

do_run_test IS-07-01 --host "${host_ip}" --port 1080 --version v1.0

do_run_test IS-07-02 --host "${host_ip}" "${host_ip}" "${host_ip}" --port 1080 1080 1080 --version v1.3 v1.1 v1.0

do_run_test IS-08-01 --host "${host_ip}" --port 1080 --version v1.0 --selector null

do_run_test IS-08-02 --host "${host_ip}" "${host_ip}" --port 1080 1080 --version v1.3 v1.0 --selector null null

do_run_test IS-09-02 --host "${host_ip}" null --port 0 0 --version null v1.0

# Run Registry tests (leave Node running)
"${registry_command}" "{\"pri\":0,\"http_port\":8088,\"domain\":\"local.\",\"logging_level\":-40}" > ${results_dir}/registryoutput 2>&1 &
REGISTRY_PID=$!
# short delay to give the Registry a chance to start up and the Node a chance to register before running the Registry test suite
sleep 2
# add a persistent Query WebSocket API subscription before running the Registry test suite
curl "http://localhost:8088/x-nmos/query/v1.3/subscriptions" -H "Content-Type: application/json" -d "{\"max_update_rate_ms\": 100, \"resource_path\": \"/nodes\", \"params\": {\"label\": \"host1\"}, \"persist\": true, \"secure\": false}"

do_run_test IS-04-02 --host "${host_ip}" "${host_ip}" --port 8088 8088 --version v1.3 v1.3

do_run_test IS-09-01 --host "${host_ip}" --port 8088 --version v1.0

# Stop Node and Registry
kill $NODE_PID || echo "node not running"
kill $REGISTRY_PID || echo "registry not running"

