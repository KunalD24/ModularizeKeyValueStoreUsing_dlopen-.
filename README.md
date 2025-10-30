# Mini Key-Value Store using Unix Domain Sockets and dlopen

This project implements a simple **key-value store** system using **Unix domain sockets** in C. It includes a **server**, a **client**, and a **dynamic library** that manages the key-value storage.

## Commands Used
```bash
gcc -fPIC -shared -o libkvstore.so kv_store_dynamic.c
```
```bash
gcc server.c -ldl -o kv_server
```
```bash
./kv_server
```
```bash
gcc client.c -o kv_client
```
```bash
./kv_client
```
