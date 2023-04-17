#!/bin/bash

ip_address="0.0.0.0"
port="2023"
test_dir="./tests"
mode="tcp" # Change to "tcp" for TCP mode
log_file="test_log.txt"

echo -n "" > "${log_file}"

for test_input in "${test_dir}/${mode}"/*_input.txt; do
    test_output="${test_input%_input.txt}_output.txt"
    test_name="$(basename "${test_input}" _input.txt)"

    if [ "${mode}" == "udp" ]; then
        payload=$(cat "${test_input}")

        response=$(echo -ne "${payload}\n" | timeout 1s ./ipkcpc -h ${ip_address} -p ${port} -m udp)
    else
        payload=$(cat "${test_input}")

        response=$(echo -n "${payload}\n" | nc -w1 ${ip_address} ${port})
    fi

    expected_output=$(cat "${test_output}")
    if [ "${response}" == "${expected_output}" ]; then
        echo "${test_name}: PASS" >> "${log_file}"
    else
        echo "${test_name}: FAIL" >> "${log_file}"
        echo "  Input   : ${payload}" >> "${log_file}"
        echo "  Expected: ${expected_output}" >> "${log_file}"
        echo "  Actual  : ${response}" >> "${log_file}"
    fi
done

cat "${log_file}"
