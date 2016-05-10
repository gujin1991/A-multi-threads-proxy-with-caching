# A-multi-threads-proxy-with-caching
* A very simple concurrent proxy with caching.
* Every time a client sent a request to the proxy,
* the proxy create a new thread to deal with it.
* If the request have been sent before, the proxy
* gets the data from cache. Otherwise it sent the
* request to the server and cache the return data.
* Finally the proxy respond the client with requested
* data. The proxy only works with GET method. The
* cache adopt LRU strategy.
