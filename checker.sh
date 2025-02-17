#!/bin/bash

url=${INPUT_URL}
api_key=${INPUT_API_KEY}
model=${INPUT_MODEL}
repo_path=${INPUT_REPO_PATH}

if [ ! $url ]; then
    echo "url is required"
    exit -1
fi

if [ ! $api_key ]; then
    echo "api key is required"
    exit -2
fi

if [ ! -d "$repo_path" ]; then
    echo "repo path does not exist"
    exit -3
fi

if [ ! $model ]; then
    echo "model is required"
    exit -4
fi

function check_code(){
    code=$(cat "$1")
    system_message='You are a '${2}' code style evaluator that checks code against style guidelines.
EXAMPLE JSON OUTPUT:
{
    \"violations\": [
        { \"line number\": \"reason\" },
        { \"line number\": \"reason\" },
    ],
    \"score\":0
}'

    user_message="Style Guide: 

${3} 

Code to evaluate: 

${code}"
    user_message=$(echo $user_message | sed 's/{/\\{/g' | sed 's/}/\\}/g' | sed 's/\"/\\"/g' | sed 's/\\\\"/\\"/g') 

    json='{
    "response_format": {"type": "json_object"},
    "model": "'$model'",
    "messages": [
        {"role": "system", "content": "'$system_message'"},
        {"role": "user", "content": "'$user_message'"}
    ],
    "stream": false
}'

    response=$(curl -s -X POST "$url" \
        -H "Content-Type: application/json" \
        -H "Authorization: Bearer $api_key" \
        --data "${json}")
    
    echo $1
    # 检查是否有错误
    error=$(echo $response | jq -r '.error // empty')
    if [ ! -z "$error" ]; then
        echo "Error: $error"
        continue
    fi

    # 提取内容
    content=$(echo $response | jq -r '.choices[0].message.content // "No content found"')
    echo "$content"
}

files=$(git ls-files '*.rs')
for file in ${files[*]}
do
    check_code $file "Rust" "Rust"
done

files=$(git ls-files '*.go')
for file in ${files[*]}
do
    check_code $file "Golang" "Golang"
done

files=$(git ls-files '*.py')
for file in ${files[*]}
do
    check_code $file "Python" "Python"
done

files=$(git ls-files '*.js')
for file in ${files[*]}
do
    check_code $file "JavaScript" "JavaScript"
done

files=$(git ls-files '*.ts')
for file in ${files[*]}
do
    check_code $file "TypeScript" "TypeScript"
done

files=$(git ls-files '*.sol')
for file in ${files[*]}
do
    check_code $file "Solidity" "Solidity"
done

files=$(git ls-files '*.cpp')
for file in ${files[*]}
do
    check_code $file "C++" "Google C++"
done

files=$(git ls-files '*.h')
for file in ${files[*]}
do
    check_code $file "C++" "Google C++"
done

exit 0
