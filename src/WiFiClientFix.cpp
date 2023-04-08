#include "WiFiClientFix.h"
#include <WiFi.h>
#include <lwip/sockets.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>

#define WIFI_CLIENT_FLUSH_BUFFER_SIZE    (1024)

#define WIFI_CLIENT_DEF_CONN_TIMEOUT_MS  (3000)

class WiFiClientSocketHandle {
private:
    int sockfd;

public:
    WiFiClientSocketHandle(int fd):sockfd(fd)
    {
    }

    ~WiFiClientSocketHandle()
    {
        close(sockfd);
    }

    int fd()
    {
        return sockfd;
    }
};

class WiFiClientRxBuffer {
private:
        size_t _size;
        uint8_t *_buffer;
        size_t _pos;
        size_t _fill;
        int _fd;
        bool _failed;

        size_t r_available()
        {
            if(_fd < 0){
                return 0;
            }
            int count;
#ifdef ESP_IDF_VERSION_MAJOR
            int res = lwip_ioctl(_fd, FIONREAD, &count);
#else
            int res = lwip_ioctl_r(_fd, FIONREAD, &count);
#endif
            if(res < 0) {
                _failed = true;
                return 0;
            }
            return count;
        }

        size_t fillBuffer()
        {
            if(!_buffer){
                _buffer = (uint8_t *)malloc(_size);
                if(!_buffer) {
                    log_e("Not enough memory to allocate buffer");
                    _failed = true;
                    return 0;
                }
            }
            if(_fill && _pos == _fill){
                _fill = 0;
                _pos = 0;
            }
            if(!_buffer || _size <= _fill || !r_available()) {
                return 0;
            }
            int res = recv(_fd, _buffer + _fill, _size - _fill, MSG_DONTWAIT);
            if(res < 0) {
                if(errno != EWOULDBLOCK) {
                    _failed = true;
                }
                return 0;
            }
            _fill += res;
            return res;
        }

public:
    WiFiClientRxBuffer(int fd, size_t size=1436)
        :_size(size)
        ,_buffer(NULL)
        ,_pos(0)
        ,_fill(0)
        ,_fd(fd)
        ,_failed(false)
    {
        //_buffer = (uint8_t *)malloc(_size);
    }

    ~WiFiClientRxBuffer()
    {
        free(_buffer);
    }

    bool failed(){
        return _failed;
    }

    int read(uint8_t * dst, size_t len){
        if(!dst || !len || (_pos == _fill && !fillBuffer())){
            return _failed ? -1 : 0;
        }
        size_t a = _fill - _pos;
        if(len <= a || ((len - a) <= (_size - _fill) && fillBuffer() >= (len - a))){
            if(len == 1){
                *dst = _buffer[_pos];
            } else {
                memcpy(dst, _buffer + _pos, len);
            }
            _pos += len;
            return len;
        }
        size_t left = len;
        size_t toRead = a;
        uint8_t * buf = dst;
        memcpy(buf, _buffer + _pos, toRead);
        _pos += toRead;
        left -= toRead;
        buf += toRead;
        while(left){
            if(!fillBuffer()){
                return len - left;
            }
            a = _fill - _pos;
            toRead = (a > left)?left:a;
            memcpy(buf, _buffer + _pos, toRead);
            _pos += toRead;
            left -= toRead;
            buf += toRead;
        }
        return len;
    }

    int peek(){
        if(_pos == _fill && !fillBuffer()){
            return -1;
        }
        return _buffer[_pos];
    }

    size_t available(){
        return _fill - _pos + r_available();
    }
};

WiFiClientFixed::WiFiClientFixed()
{
	_connected = false;
	_timeout = WIFI_CLIENT_DEF_CONN_TIMEOUT_MS;
	next = NULL;
}

WiFiClientFixed::WiFiClientFixed(int fd)
{
	_connected = true;
	_timeout = WIFI_CLIENT_DEF_CONN_TIMEOUT_MS;
	next = NULL;

    clientSocketHandle.reset(new WiFiClientSocketHandle(fd));
    _rxBuffer.reset(new WiFiClientRxBuffer(fd));
}

WiFiClientFixed::~WiFiClientFixed()
{
    stop();
}

void WiFiClientFixed::flush() {
	int res;
	size_t a = available(), toRead = 0;
	if (!a) {
		return;//nothing to flush
	}
	auto *buf = (uint8_t *) malloc(WIFI_CLIENT_FLUSH_BUFFER_SIZE);
	if (!buf) {
		return;//memory error
	}
	while (a) {
		// override broken WiFiClient flush method, ref https://github.com/espressif/arduino-esp32/issues/6129#issuecomment-1237417915
        // https://github.com/espressif/arduino-esp32/issues/6129#issuecomment-1418051304
		res = read(buf, min(a, (size_t)WIFI_CLIENT_FLUSH_BUFFER_SIZE));
		if (res < 0) {
			log_e("fail on fd %d, errno: %d, \"%s\"", fd(), errno, strerror(errno));
			stop();
			break;
		}
		a -= res;
	}
	free(buf);
}

void WiFiClientSecureFixed::flush() {
	int res;
	size_t a = available(), toRead = 0;
	if (!a) {
		return;//nothing to flush
	}
	auto *buf = (uint8_t *) malloc(WIFI_CLIENT_FLUSH_BUFFER_SIZE);
	if (!buf) {
		return;//memory error
	}
	while (a) {
		// override broken WiFiClient flush method, ref https://github.com/espressif/arduino-esp32/issues/6129#issuecomment-1237417915
        // https://github.com/espressif/arduino-esp32/issues/6129#issuecomment-1418051304
		res = read(buf, min(a, (size_t)WIFI_CLIENT_FLUSH_BUFFER_SIZE));
		if (res < 0) {
			log_e("fail on fd %d, errno: %d, \"%s\"", fd(), errno, strerror(errno));
			stop();
			break;
		}
		a -= res;
	}
	free(buf);
}
