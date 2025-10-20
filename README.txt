#
# This file should include your name, which course you are registered for (471 or 598-J)
# and any special instructions or notes requried for building and running your RFT-Client
#
Rachel Cavalier
598-J, no special instructions

I tested my program with many different combinations of window sizes and timeouts. 
By far the slowest encountered was a window size of 10 with a timeout of 200, producing 78Bpms of goodput and 
93 Bpms of throughput. The highest value I encountered was with small windows and small timeouts. When I ran with 
N=5 and timeout 5ms, my goodput was 2250Bpms and my throughput was 2549Bpms. Also, having a small timeout seemed 
to result in a higher throughput and goodput despite the window size. For example, with a window size of 200 but 
still a small, 5ms timeout, the program achieved 1930Bpms of goodput and 10233Bpms of throughput.

We can see in the last example that having a combination of large window size and small timeout results in a lot 
of retransmissions, since the throughput was a magnitude larger. This aligns with what we see in the textbook, since 
a smaller timeout will lead to more retransmissions as small delays will result in the packets not being acked in time 
before they are resent. In the textbook, the window size is a limitation as it must wait for an acknowledgement 
after sending each datagram to move forward. So theoretically, a larger window would result in a more efficient 
sending of data (besides with loss/corruption). This wasn’t necessarily reflected in my program, it seemed the 
timer had a larger effect on throughput and goodput. My conclusion for the timer is that the smaller numbers are 
the most efficient, reflecting the principle of the ideal timeout being slightly over the RTT. The window, in this case, 
could be small and still achieve the shortest runtime and highest throughput and goodput. I tested these sizes with 1% 
loss and corruption on a 1MB file, which is likely why the larger window sizes were less efficient (because the larger 
window size required more retransmission).

