#Cotex-M0 UART paramters finder


f_cpu = 48000000
f_cpu_error_PPM = 10000 # in PPM, 1% = 10000 PPM. The internal oscilator has 
target_baud_1 = 19200
target_baud_2 = 19231
acceptable_total_error = 2 #percent


target_baud = (target_baud_1 + target_baud_2) / 2.0


acceptable_baud_error = acceptable_total_error - f_cpu_error_PPM / 10000.0

minb = target_baud * (1.0 - acceptable_baud_error/100.0)
maxb = target_baud * (1.0 + acceptable_baud_error/100.0)

print 'For a CPU running at', "{0:.6f}".format(f_cpu/1000000.0), 'MHz +/-', "{0:.6f}".format(f_cpu/float(f_cpu_error_PPM)/10000.0) ,'MHz [Clock accuracy of', f_cpu_error_PPM, "PPM (", "{0:.6f}".format(f_cpu_error_PPM / 10000.0), "%)]"
print 'Looking for', target_baud, 'bps at a TOTAL CUMULATED ERROR of +/-' ,"{0:.3f}".format(acceptable_total_error)
print 'Acceptable baudrate error is therefore', "{0:.3f}".format(acceptable_baud_error) ,'% (min =', minb, 'bps, max=', maxb,'bps)'


def check_baudrate_no_fdr(clk_div, latch):
    f_clk = f_cpu / float(clk_div)
    res_clk = f_clk / (16.0 * latch)
    #print res_clk
    if maxb > res_clk > minb  :
        show(res_clk, clk_div, latch,0, 1)

results = dict()

def show(res_clk, clk_div, latch, div, mul):
    err_percent = (res_clk/float(target_baud)-1)*100
    if not float('%.3f'%(err_percent)) in results:  #make it unique, keep the first one...
        results[ float('%.3f'%(err_percent)) ] = ( res_clk, clk_div, latch, div, mul )
        

for pclk_div in xrange(1,255):
    print pclk_div,
    
    #divisor latch = 1, fractional stuff cannot be used 
    check_baudrate_no_fdr( pclk_div, 1 )

    #divisor latch = 2, fractional stuff cannot be used 
    check_baudrate_no_fdr( pclk_div, 2 )
    f_clk = f_cpu / pclk_div

    if f_clk/16.0 < minb: # trim useless stuff
        break
    
    for divisor_latch in xrange(3, 65535):
        check_baudrate_no_fdr( pclk_div, divisor_latch )

        tmp = f_clk / (16.0 * divisor_latch)

        if float(tmp) < minb: # trim useless stuff
            continue
        
        for mul_val in xrange(1, 16):
            for divadd_val in xrange(1, mul_val):
                res_clk =    tmp / (1.0 + divadd_val / float(mul_val)) 
                #print res_clk
                if maxb > res_clk > minb  :
                    show(res_clk,pclk_div, divisor_latch, divadd_val, mul_val)

# sort the result
print '\n\nResults'

ordered_key = sorted ([ (abs(x),x) for x in results.keys()])

for abs_k,err_percent in ordered_key:
    res_clk,clk_div, latch, div,mul = results[err_percent]
    print 'res=', int(res_clk), '\tclkdiv=',clk_div,'\terror%=', "{0: .3f}".format( err_percent )  , '\tlatch=', latch,'\t div=', div,'\t mul=',mul


