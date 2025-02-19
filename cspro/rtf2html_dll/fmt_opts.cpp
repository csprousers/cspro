﻿/* This file was modified from the original rtf2html 0.2.0 source by
   Josh Handley for OECD/Paris21/IHSN.
   Changes are marked with "JH" in comments.

   All changes Copyright (c) 2009 OECD.
   Released under original rtf2html license (LGPL 2.1)
   See COPYING.LESSER for details.
*/

#include "fmt_opts.h"

std::string formatting_options::get_par_str() const
{
   std::string style;
   switch (papAlign)
   {
   case formatting_options::align_right:
      style+="text-align:right;";
      break;
   case formatting_options::align_center:
      style+="text-align:center;";
      break;
   case formatting_options::align_justify:
      style+="text-align:justify;";
   }
   if (papFirst!=0)
   {
      style+="text-indent:";
      style+=from_int(papFirst);
      style+="pt;";
   }
   if (papLeft!=0)
   {
      style+="margin-left:";
      style+=from_int(papLeft);
      style+="pt;";
   }
   if (papRight!=0)
   {
      style+="margin-right:";
      style+=from_int(papRight);
      style+="pt;";
   }
   if (papBefore!=0)
   {
      style+="margin-top:";
      style+=from_int(papBefore);
      style+="pt;";
   }
   if (papAfter!=0)
   {
      style+="margin-bottom:";
      style+=from_int(papAfter);
      style+="pt;";
   }
   if (style.empty())
      return std::string("<p>");
   else
   {
      style.insert(0, "<p style=\"");
      return style+"\">";
   }
}

std::string formatter::format(const formatting_options &_opt)
{
   formatting_options last_opt, opt(_opt);
   std::string result;
   if (!opt_stack.empty())
   {
      int cnt=0;
      fo_deque::reverse_iterator i;
      for (i=opt_stack.rbegin(); i!=opt_stack.rend(); ++i)
      {
         if (*i==opt)
            break;
         ++cnt;
      }
      if (cnt==0)
         return "";
      if (i!=opt_stack.rend())
      {
         while (cnt--)
         {
            result+="</span>";
            opt_stack.pop_back();
         }
         return result;
      }
      last_opt=opt_stack.back();
   }
   if (last_opt.chpVAlign!=formatting_options::va_normal
       && last_opt.chpVAlign!=opt.chpVAlign)
   {
      int cnt=0;
      fo_deque::reverse_iterator i;
      for (i=opt_stack.rbegin(); i!=opt_stack.rend(); ++i)
      {
         if (i->chpVAlign==formatting_options::va_normal)
            break;
         ++cnt;
      }
      while (cnt--)
      {
         result+="</span>";
         opt_stack.pop_back();
      }
      last_opt=opt_stack.empty()?formatting_options():opt_stack.back();
   }
   std::string style;
   if (opt.chpBold!=last_opt.chpBold)
   {
      style+="font-weight:";
      style+=opt.chpBold?"bold":"normal";
      style+=";";
   }
   if (opt.chpItalic!=last_opt.chpItalic)
   {
      style+="font-style:";
      style+=opt.chpItalic?"italic":"normal";
      style+=";";
   }
   if (opt.chpUnderline!=last_opt.chpUnderline)
   {
      style+="text-decoration:";
      style+=opt.chpUnderline?"underline":"none";
      style+=";";
   }
   if (opt.chpVAlign!=formatting_options::va_normal)
      opt.chpFontSize=(int)(0.7*(opt.chpFontSize?opt.chpFontSize:24));
   if (opt.chpFontSize!=last_opt.chpFontSize)
   {
      style+="font-size:";
      int size_pt = opt.chpFontSize / 2;
      int size_px = 4 * size_pt / 3;
      style+=from_int(size_px);
      style+="px;";
   }
   if (opt.chpVAlign!=last_opt.chpVAlign)
   {
      style+="vertical-align:";
      style+=opt.chpVAlign==formatting_options::va_sub?"sub":"super";
      style+=";";
   }

   auto color_to_hex = [](color selected_color) -> std::string
   {
       // use black as the default color
       if( selected_color == color() )
           selected_color.r = selected_color.g = selected_color.b = 0;

      return "#" + hex(selected_color.r & 0xFF) + hex(selected_color.g & 0xFF) + hex(selected_color.b & 0xFF);
   };

   if (opt.chpFColor!=last_opt.chpFColor)
   {
      style+="color:" + color_to_hex(opt.chpFColor) + ";";
   }
   if (opt.chpBColor!=last_opt.chpBColor)
   {
      style+="background-color:" + color_to_hex(opt.chpBColor) + ";";
   }
   if (opt.chpHighlight!=last_opt.chpHighlight)
   {
      style+="background-color:";
      switch (opt.chpHighlight)
      {
      case 0: style+="Window"; break;
      case 1: style+="black"; break;
      case 2: style+="blue"; break;
      case 3: style+="aqua"; break;
      case 4: style+="lime"; break;
      case 5: style+="fuchsia"; break;
      case 6: style+="red"; break;
      case 7: style+="yellow"; break;
      case 9: style+="navy"; break;
      case 10: style+="teal"; break;
      case 11: style+="green"; break;
      case 12: style+="purple"; break;
      case 13: style+="maroon"; break;
      case 14: style+="olive"; break;
      case 15: style+="gray"; break;
      case 16: style+="silver"; break;
      }
      style+=";";
   }
   if (opt.chpFont!=last_opt.chpFont)
   {
      style+="font-family:'";
      style+=opt.chpFont.name.empty()?"serif":opt.chpFont.name;
      style+="'";
      switch (opt.chpFont.family)
      {
      case font::ff_serif: style+=", serif"; break;
      case font::ff_sans_serif: style+=", sans-serif"; break;
      case font::ff_cursive: style+=", cursive"; break;
      case font::ff_fantasy: style+=", fantasy"; break;
      case font::ff_monospace: style+=", monospace"; break;
      }
      style+=";";
   }
   opt_stack.push_back(opt);
   return result+"<span style=\""+style+"\">";
}

std::string formatter::close()
{
   std::string result;
   for (fo_deque::iterator i=opt_stack.begin(); i!=opt_stack.end(); ++i)
      result+="</span>";
   return result;
}
