#include "nat64/mod/out_stream.h"
#include "net/netlink.h"
#include "nat64/comm/types.h"


static void flush(struct out_stream *stream, __u16 nlmsg_type)
{
	struct sk_buff *skb_out;
	struct nlmsghdr *nl_hdr_out;
	int res;

	skb_out = nlmsg_new(NLMSG_ALIGN(stream->buffer_len), GFP_ATOMIC);
	if (!skb_out) {
		log_err(ERR_ALLOC_FAILED, "Failed to allocate a response skb to the user.");
		return;
	}

	nl_hdr_out = nlmsg_put(skb_out,
			0, // src_pid (0 = kernel)
			stream->request_hdr->nlmsg_seq, // seq
			nlmsg_type, // type
			stream->buffer_len, // payload len
			NLM_F_MULTI); // flags.
	memcpy(nlmsg_data(nl_hdr_out), stream->buffer, stream->buffer_len);
	// NETLINK_CB(skb_out).dst_group = 0;

	res = nlmsg_unicast(stream->socket, skb_out, stream->request_hdr->nlmsg_pid);
	if (res < 0)
		log_err(ERR_NETLINK, "Error code %d while returning response to the user.", res);

	stream->buffer_len = 0;
}

void stream_init(struct out_stream *stream, struct sock *nl_socket, struct nlmsghdr *nl_hdr)
{
	stream->socket = nl_socket;
	stream->request_hdr = nl_hdr;
	stream->buffer_len = 0;
}

void stream_write(struct out_stream *stream, void *payload, int payload_len)
{
	if (payload == NULL || payload_len == 0)
		return;

	// TODO (fine) if payload_len > BUFFER_SIZE, this will go downhill.
	// Will never happen in this project, hence the low priority.
	if (stream->buffer_len + payload_len > BUFFER_SIZE)
		flush(stream, 0);

	memcpy(stream->buffer + stream->buffer_len, payload, payload_len);
	stream->buffer_len += payload_len;
	/* There might still be room in the buffer, so don't flush it yet. */
}

void stream_close(struct out_stream *stream)
{
	flush(stream, NLMSG_DONE);
}
