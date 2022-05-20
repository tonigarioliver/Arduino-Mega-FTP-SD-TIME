#include "pt100rtd.h"

/*******************************************************************
*	 pt100rtd contructor --
*
*******************************************************************/

pt100rtd::pt100rtd() { ; }


/**********************************************************************
** Function Name:	search_pt100_list
**
** Description:		binary search
**  	if match
**  	    return index of a match
**  	if no match
**  	    return index of the smallest table value > key
** 
**	usually requires the maximum of log2(1051) probes, ==10, 
**	when search key is not an exact match.
**
**	Note: search must not return index == 0.
**	Calling function must exclude boundary cases
**	where (ohmsX100 <= table[0]).
** 
** Parameters:
**		uint16_t ohmsX100
**
** Uses:
** Returns:	int index of nearest resistance value
** Creation: 1/26/2017 4:48a Daniel R. Haney
**********************************************************************/

int pt100rtd::search_pt100_list(uint16_t ohmsX100)
{
    int lower = 0;
    int upper = PT100_TABLE_MAXIDX;
    int mid = (lower + upper) / 2;

    do
    {
	uint16_t pt100val = pgm_read_word_near(&Pt100_table[mid]);
		
	if (pt100val == ohmsX100)
	{
	    break;
	}
	else if (pt100val < ohmsX100)
	{
	    lower = mid + 1;
	}
	else
	{
	    upper = mid;
	}
		
	mid = (lower + upper) / 2;
		
    } while (lower < upper);
	// falls through on last mismatch

    return(mid);
}

/**********************************************************************
** Function Name:	ohmsX100_to_celsius
**
** Description:
** 	Look up (unsigned short int)(Pt100 resistance * 100) in table.
**  	Interpolate temperature for intermediate resistances.
** 
**  	Calling function must exclude boundary cases where
**  	ohmsX100 <= table[0] && ohmsX100 >= table[MAX]
** 
** Parameters:
**	uint16_t Rrtd = 100 * (Pt100 RTD resistance in ohms)
**
** Uses:	Pt100_table
** Returns:	float temperature celsius
**
** Creation: 1/26/2017 10:41a Daniel R. Haney
**********************************************************************/
float pt100rtd::ohmsX100_to_celsius (uint16_t ohmsX100)
{
    float R_upper, R_lower;
    float hundredths = 0; 		// STFU flag for avr-gcc
    int iTemp = 0;
    float celsius;

    int index = search_pt100_list(ohmsX100);
	
    // The minimum integral temperature
    iTemp = index - 1 + CELSIUS_MIN;
	
    // fetch floor() and ceiling() resistances since
    // key = intermediate value is the most likely case.

    // ACHTUNG!  (index == 0) is forbidden!
    R_lower = pgm_read_word_near(&Pt100_table[index - 1]);
    R_upper = pgm_read_word_near(&Pt100_table[index]);

    // if key == table entry, temp is an integer degree
    if (ohmsX100 == R_upper)
    {
	iTemp++;
	hundredths = 0;
    }
    // an intermediate resistance is the common case
    else if (ohmsX100 < R_upper)
    {
	hundredths = ((100 * (ohmsX100 - R_lower)) / (R_upper - R_lower));
    }
    // two unlikely cases are included for disaster recovery
    else if (ohmsX100 > R_upper) /*NOTREACHED*/  /*...unless list search was dain bramaged */
    {
	iTemp++;
	// risks index+1 out of range
	uint16_t Rnext = pgm_read_word_near(&Pt100_table[index + 1]);
	hundredths = (100 * (ohmsX100 - R_upper)) / (Rnext - R_upper);
    }
    else	/*NOTREACHED*/  /*...except in cases of excessive tweakage at 2:30am */
    {
	hundredths = ((100 * (ohmsX100 - R_lower)) / (R_upper - R_lower));
    }

    celsius  = (float)iTemp + (float)hundredths / 100.0;

    return(celsius );
}


/**********************************************************************
** Function Name:	celsius (uint16_t)
** Function Name:	celsius (float)
**
** Description:
** 		return celsius temperature for a given Pt100 RTD resistance
**
**		This wrapper function excludes boundary cases where
**  		ohmsX100 <= table[0] && ohmsX100 >= table[MAX]
** 
** Creation: 2/18/2017 2:29p Daniel R. Haney
**********************************************************************/

// Uses minimally-processed ADC binary output,
// an unsigned 16 bit integer == (ohms * 100).

float pt100rtd::celsius (uint16_t ohmsX100)
{
    // clip underflow	
    if (ohmsX100 <= pgm_read_word_near(&Pt100_table[0]))
    {
	// return min boundary temperature
	return((float) CELSIUS_MIN);
    }
    // clip overflow
    else if (ohmsX100 >= pgm_read_word_near(&Pt100_table[PT100_TABLE_MAXIDX]))
    {
	// return max boundary temperature
	return((float) CELSIUS_MAX);
    }
    else
    {
	return(pt100rtd::ohmsX100_to_celsius(ohmsX100));
    }
}


// Uses a floating point resistance value.

float pt100rtd::celsius (float rtd_ohms)
{
    // convert to unsigned short
    uint16_t ohmsX100 = (uint16_t) floor(rtd_ohms * 100.0);
    return pt100rtd::celsius(ohmsX100);
}

/*END*/
