#include "nat64/mod/handling_hairpinning.h"
#include "nat64/mod/pool4.h"
#include "nat64/mod/filtering_and_updating.h"
#include "nat64/mod/compute_outgoing_tuple.h"
#include "nat64/mod/translate_packet.h"
#include "nat64/mod/send_packet.h"


bool is_hairpin(struct tuple *outgoing)
{
	return (outgoing->l3_proto == PF_INET6) && pool4_contains(&outgoing->dst.addr.ipv4);
}

bool handling_hairpinning(struct sk_buff *skb_in, struct tuple *tuple_in)
{
	struct sk_buff *skb_out = NULL;
	struct tuple tuple_out;

	log_debug("Step 5: Handling Hairpinning...");

	if (filtering_and_updating(skb_in, tuple_in) != NF_ACCEPT)
		goto free_and_fail;
	if (!compute_out_tuple_4to6(tuple_in, skb_in, &tuple_out))
		goto free_and_fail;
	if (!translating_the_packet_4to6(&tuple_out, skb_in, &skb_out))
		goto free_and_fail;
	if (!send_packet_ipv6(skb_out))
		goto fail;

	log_debug("Done step 5.");
	return true;

free_and_fail:
	kfree_skb(skb_out);
	// Fall through.

fail:
	return false;
}
