#include "request_bind.hpp"

extern "C"
bool oomph_request_test(oomph::fort::frequest_type *freq)
{
    if(freq->recv_request){
        oomph::recv_request *req = reinterpret_cast<oomph::recv_request*>(freq->data);
        return req->test();
    } else {
        oomph::send_request *req = reinterpret_cast<oomph::send_request*>(freq->data);
        return req->test();
    }
}

extern "C"
void oomph_request_wait(oomph::fort::frequest_type *freq)
{
    if(freq->recv_request){
        oomph::recv_request *req = reinterpret_cast<oomph::recv_request*>(freq->data);
        req->wait();
    } else {
        oomph::send_request *req = reinterpret_cast<oomph::send_request*>(freq->data);
        req->wait();
    }
}

extern "C"
bool oomph_request_cancel(oomph::fort::frequest_type *freq)
{
    if(freq->recv_request){
        oomph::recv_request *req = reinterpret_cast<oomph::recv_request*>(freq->data);
        return req->cancel();
    } else {
        return false;
    }
}
