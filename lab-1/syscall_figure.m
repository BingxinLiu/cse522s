x = [6 7 8 9];
y1 = [-0.432757	-0.440859	-0.438690	-0.420707];
y1err = [0.037436	0.044871	0.031400	0.025197];
y2 = [-0.407513	-0.434614	-0.427521	-0.427519];
y2err = [0.135077	0.135403	0.090956	0.106487];
figure
plot(x, y1, x, y2);
errorbar(x, y1, y1err);
hold on;
errorbar(x, y2, y2err);
title("System Call Time v.s Matrix Size");
xlabel("Matrix Size(log)");
ylabel("System Call Time(log)");
legend("demand\_paging", "pre-paging");


