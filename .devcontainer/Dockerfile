FROM wokwi/builder-clang-wasm

USER root
RUN apk add bash gcompat libc6-compat curl

RUN curl -L https://wokwi.com/ci/install.sh | sh