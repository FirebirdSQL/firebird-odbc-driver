/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// TemplateConvert.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(_TemplateConvert_h_)
#define _TemplateConvert_h_

namespace OdbcJdbcLibrary {

template <typename T>
class ConvertFloatToString
{
public:

	ConvertFloatToString( double value, T *string, int size, int *length, int precision = 15, T POINT_DIV = '.' )
	{
	#define MAXDIGITS 512
		const int maxDecimalExponent = 308;
		T temp[64];
		T *dst = temp;
		int numDigits = precision - 1;
		int realDigits;
		double valInt, valFract;
		T *pt, *pt1;
		T buf[MAXDIGITS];
		int sign;
		bool copy = false;
		int &len = *length;

		len = 0;

		if ( !size )
			return;

		if ( size >= 24 )
			dst = string;
		else
			copy = true;

		realDigits = 0;
		sign = 0;

		if ( value < 0 )
		{
			sign = 1;
			value = -value;
		}

		value = modf ( value, &valInt );

		if ( valInt != 0 )
		{
			pt = pt1 = &buf[MAXDIGITS - numDigits - 1];
			T * end = buf + 1;

			while ( valInt != 0 )
			{
				valFract = modf ( valInt / 10, &valInt );
				*--pt1 = (int)( ( valFract + 0.03 ) * 10 ) + (T)'0';
				realDigits++;

				if ( realDigits > maxDecimalExponent )
				{
					*pt1 = (T)'1';
					break;
				}
			}

			if ( realDigits > numDigits ) // big number
			{
				roundStringNumber ( pt1, numDigits, realDigits );

				int ndig = numDigits;

				pt = dst;

				if ( sign )
					*pt++ = (T)'-';
				
				*pt++ = *pt1++;
				*pt++ = POINT_DIV;

				while ( --ndig )
					*pt++ = *pt1++;

				end = pt - 1;
				while ( *end == (T)'0' ) --end;
				
				if ( *end == POINT_DIV )
					pt = end;
				else
					pt = end + 1;

				*pt++ = (T)'e';
				*pt = (T)'+';

				ndig = realDigits - 1;

				int n;
				for ( n = 3, pt += n; ndig; ndig /= 10, --n)
					*pt-- = (T)'0' + (T) (ndig % 10);

				while ( n-- )
					*pt-- = (T)'0';

				pt += 4;
				*pt = (T)'\0';

				len = pt - dst;
				return;
			}

			// normal number
			end = pt1 + numDigits;
			
			while ( pt <= end )
			{
				value *= 10;
				value = modf ( value, &valFract );
				*pt++ = (int)valFract + (T)'0';
			}

			*pt = (T)'\0';

			roundStringNumber ( pt1, numDigits, realDigits );

			*(pt-1) = (T)'\0';
			pt = dst;

			if ( sign )
				*pt++ = '-';

			int n = realDigits;

			while ( n-- )
				*pt++ = *pt1++;

			n = numDigits - realDigits;
			end = pt1 + (n - 1);

			while ( n > 0 && *end == (T)'0' ) --end, --n;

			if ( !n )
				*pt = (T)'\0';
			else
			{
				*(end + 1) = (T)'\0';
				*pt++ = POINT_DIV;
				while ( (*pt++ = *pt1++) );
				--pt;
			}
		} 
		else if ( value > 0 ) 
		{   // shift to left number 0.0000122 to 0.122
			while ( ( valFract = value * 10 ) < 1 ) 
			{
				value = valFract;
				realDigits--;
			}

			T * beg = buf + 1;
			pt1 = buf + numDigits + 1;
			pt = buf + 1;
			
			while ( pt <= pt1 )
			{
				value *= 10;
				value = modf ( value, &valFract );
				*pt++ = (int)valFract + (T)'0';
			}

			*pt = (T)'\0';

			roundStringNumber ( beg, numDigits, realDigits );

			*--pt = (T)'\0';
			--pt;

			while ( pt > beg && *pt == (T)'0' ) 
				*pt-- = (T)'\0';

			pt = dst;

			if ( sign )
				*pt++ = (T)'-';

			if ( realDigits == 1 )
			{
				while ( (*pt++ = *beg++) );
			}
			else if ( realDigits >= -3 )
			{
				*pt++ = (T)'0';

				if ( *beg > (T)'0' )
				{
					int n = realDigits;

					*pt++ = POINT_DIV;

					while ( n++ )
						*pt++ = (T)'0';

					while ( (*pt++ = *beg++) );
					--pt;
				}
				else
					*pt = (T)'\0';
			}
			else
			{
				*pt++ = *beg++;

				if ( *beg )
				{
					*pt++ = POINT_DIV;

					while ( *beg )
						*pt++ = *beg++;
				}

				*pt++ = (T)'e';
				*pt = (T)'-';

				int ndig = -realDigits + 1;
				int n;

				for ( n = 3, pt += n; ndig; ndig /= 10, --n)
					*pt-- = (T)'0' + (T) (ndig % 10);

				while ( n-- )
					*pt-- = (T)'0';

				pt += 4;
				*pt = (T)'\0';
			}
		}
		else
		{
			pt = dst;
			*pt++ = (T)'0';
			*pt = (T)'\0';
		}

		len = pt - dst;

		if ( copy )
		{
			len = MIN ( len, size - 1 );
			memcpy ( string, temp, len * sizeof(T) );
			string[len] = (T)'\0';
		}
	}

	void roundStringNumber( T *& strNumber, int numDigits, int &realDigits )
	{
		T * &chBeg = strNumber;
		T * chEnd = chBeg + numDigits;

		if ( *chEnd >= (T)'5' )
		{
			++*--chEnd;

			while ( *chEnd > (T)'9' ) 
			{
				*chEnd = (T)'0';
				if ( chEnd > chBeg )
					++*--chEnd;
				else
				{
					*--chBeg = (T)'1';
					++realDigits;
				}
			}
		}
	}

};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_TemplateConvert_h_)
