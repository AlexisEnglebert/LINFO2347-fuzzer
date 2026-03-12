#!/bin/bash
for i in {0..5}
do
  echo "Running $i th extractor"
  make && ./fuzzer extractors/bug$i
done
