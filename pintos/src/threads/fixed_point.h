#define F (1 << 14)
#define INT_MAX ((1<<31)-1)
#define INT_MIN (-1(1<<31))

int int_to_fp (int n);
int fp_to_int (int x);
int add_fp (int x, int y);
int add_mixed (int x, int y);
int sub_fp (int x, int y);
int sub_mixed (int x, int y);
int multi_fp (int x, int y);
int multi_mixed (int x, int y);
int div_fp (int x, int y);
int div_mixed (int x ,int y);

int int_to_fp (int x){
	return x * F;
}
int fp_to_int (int x){
	return x / F;
}
int add_fp(int x, int y){
	return x+y;
}
int add_mixed(int x, int y){
	return add_fp (x, int_to_fp(y));
}
int sub_fp (int x, int y){
	return x-y;
}
int sub_mixed (int x, int y){
	return sub_fp(x, int_to_fp(y));
}
int multi_fp(int x, int y){
	return (int) ((long long)x*y)/F;
}
int multi_mixed (int x, int y){
	return multi_fp(x, int_to_fp(y));
}
int div_fp (int x, int y){
	return (long long) x*F /y;
}
int div_mixed (int x, int y){
	return div_fp (x, int_to_fp(y));
}
