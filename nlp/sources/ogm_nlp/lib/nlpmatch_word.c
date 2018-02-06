/*
 *  Handling log messages for Natural Language Processing library
 *  Copyright (c) 2017 Pertimm, by Patrick Constant
 *  Dev : September 2017
 *  Version 1.0
 */
#include "ogm_nlp.h"
#include <stdlib.h>

static og_status NlpMatchWord(og_nlp_th ctrl_nlp_th, struct request_word *request_word);
static og_status NlpMatchWordInPackage(og_nlp_th ctrl_nlp_th, struct request_word *request_word, int input_length,
    unsigned char *input, struct interpret_package *interpret_package);

og_status NlpMatchWords(og_nlp_th ctrl_nlp_th)
{
  struct request_word *first_request_word = OgHeapGetCell(ctrl_nlp_th->hrequest_word, 0);
  IFN(first_request_word) DPcErr;

  for (struct request_word *rw = first_request_word; rw; rw = rw->next)
  {
    IFE(NlpMatchWord(ctrl_nlp_th, rw));
  }

  DONE;
}

static og_status NlpMatchWord(og_nlp_th ctrl_nlp_th, struct request_word *request_word)
{

  og_string string_request_word = OgHeapGetCell(ctrl_nlp_th->hba, request_word->start);
  IFN(string_request_word) DPcErr;

  unsigned char input[DPcAutMaxBufferSize + 9];
  int input_length = request_word->length;
  memcpy(input, string_request_word, input_length);
  input[input_length++] = '\1';
  input[input_length] = 0;

  char digit[DPcPathSize];
  digit[0] = 0;
  if (request_word->is_digit)
  {
    snprintf(digit, DPcPathSize, " -> %f", request_word->digit_value);
  }

  NlpLog(DOgNlpTraceMatch, "Looking for input parts for string '%s'%s:", string_request_word, digit);

  int interpret_package_used = OgHeapGetCellsUsed(ctrl_nlp_th->hinterpret_package);
  for (int i = 0; i < interpret_package_used; i++)
  {
    struct interpret_package *interpret_package = OgHeapGetCell(ctrl_nlp_th->hinterpret_package, i);
    IFN(interpret_package) DPcErr;
    IFE(NlpMatchWordInPackage(ctrl_nlp_th, request_word, input_length, input, interpret_package));
  }

  DONE;
}

static og_status NlpMatchWordInPackage(og_nlp_th ctrl_nlp_th, struct request_word *request_word, int input_length,
    unsigned char *input, struct interpret_package *interpret_package)
{
  package_t package = interpret_package->package;

  if (request_word->is_digit)
  {
    struct digit_input_part *digit_input_part_all = OgHeapGetCell(package->hdigit_input_part, 0);
    int digit_input_part_used = OgHeapGetCellsUsed(package->hdigit_input_part);
    for (int i = 0; i < digit_input_part_used; i++)
    {
      struct digit_input_part *digit_input_part = digit_input_part_all + i;
      // There is not need to have a special input part here for digit words
      IFE(NlpRequestInputPartAddWord(ctrl_nlp_th, request_word, interpret_package, digit_input_part->Iinput_part,TRUE));
    }
  }

  unsigned char out[DPcAutMaxBufferSize + 9];
  oindex states[DPcAutMaxBufferSize + 9];
  int retour, nstate0, nstate1, iout;

  if ((retour = OgAufScanf(package->ha_word, input_length, input, &iout, out, &nstate0, &nstate1, states)))
  {
    do
    {
      IFE(retour);
      int Iinput_part;
      unsigned char *p = out;
      IFE(DOgPnin4(ctrl_nlp_th->herr,&p,&Iinput_part));
      NlpLog(DOgNlpTraceMatch, "    found input part %d in request package %d", Iinput_part,
          interpret_package->self_index)
      IFE(NlpRequestInputPartAddWord(ctrl_nlp_th, request_word, interpret_package, Iinput_part,FALSE));
    }
    while ((retour = OgAufScann(package->ha_word, &iout, out, nstate0, &nstate1, states)));
  }

  DONE;
}

static og_bool str_remove(og_char_buffer *source, og_string to_replace)
{
  int to_replace_size = strlen(to_replace);
  int source_size = strlen(source);
  og_char_buffer *p = strstr(source, to_replace);
  if (p)
  {
    memmove(p, p + to_replace_size, source_size - to_replace_size - (source - p) + 1);
    IFE(str_remove(source, to_replace));
    return TRUE;
  }
  return FALSE;
}

struct lang_sep
{
  int lang;
  int country;
  og_string thousand_sep;
  og_string decimal_sep;
  og_bool used;
};

#define locale_lang_country(lang, country) (lang + DOgLangMax * country)

static struct lang_sep lang_sep_conf[] = {   //
    { DOgLangNil, DOgCountryCH, "'", "\\." , FALSE},   // *-CH  // le caractere '.' signifie n'importe quel caractère pour une regexp mais n'est pas un caractère échappé par la glib
        { DOgLangFR, DOgCountryNil, " ", ",", FALSE },   // fr-*
        { DOgLangEN, DOgCountryNil, ",", "\\.", FALSE },   // en-*

        { DOgLangNil, DOgCountryNil, "", "", FALSE }   // End
    };

static og_status getNumberSeparators(struct lang_sep* lang_conf, int locale, og_char_buffer* thousand_sep, og_char_buffer* decimal_sep)
{
  int req_lang = OgIso639_3166ToLang(locale);
  int req_country = OgIso639_3166ToCountry(locale);

  og_bool match = FALSE;
  for (struct lang_sep *conf = lang_conf; !match && (conf->lang != DOgLangNil || conf->country != DOgCountryNil);
      conf++)
  {
    if (conf->lang == DOgLangNil && conf->country == req_country)
    {
      match = TRUE;
    }
    else if (conf->lang == req_lang && conf->country == DOgCountryNil)
    {
      match = TRUE;
    }
    else if (conf->lang == req_lang && conf->country == req_country)
    {
      match = TRUE;
    }

    if (match)
    {
      snprintf(thousand_sep, 5, "%s", conf->thousand_sep);
      snprintf(decimal_sep, 5, "%s", conf->decimal_sep);
    }

  }

  // TODO default
  if (!match)
  {
    // fr by default
    snprintf(thousand_sep, 5, " ");
    snprintf(decimal_sep, 5, ",");
  }

  return match;
}

static og_bool NlpNumberParsing(og_nlp_th ctrl_nlp_th, og_string sentence, GRegex *regular_expression,
    og_string thousand_sep, double *p_value)
{
  if (p_value) *p_value = 0;
// run the regular expression
  GMatchInfo *match_info = NULL;
  og_bool match = g_regex_match(regular_expression, sentence, 0, &match_info);
  if (!match) return FALSE;

// getting the integer part
  gchar *integerpart = g_match_info_fetch(match_info, 1);

// removing thousand separators
  og_char_buffer value_buffer[DPcPathSize];
  snprintf(value_buffer, DPcPathSize, "%s", integerpart);

  str_remove(value_buffer, thousand_sep);

  gchar *decimalpart = g_match_info_fetch(match_info, 2);
  if (decimalpart)
  {
    snprintf(value_buffer + strlen(value_buffer), DPcPathSize, ".%s", decimalpart);
  }
  double tmp_value = atof(value_buffer);

// some cleaning
  g_free(integerpart);
  g_free(decimalpart);
  g_match_info_free(match_info);

  *p_value = tmp_value;

  return TRUE;
}



static og_bool NlpMatchWordGroupDigitsByLanguage(og_nlp_th ctrl_nlp_th, struct lang_sep* lang_conf)
{
  og_bool digits_grouped = FALSE;

  // getting thousand and decimal separator
  og_string thousand_sep = lang_conf->thousand_sep;
  og_string decimal_sep = lang_conf->decimal_sep;

  og_string request_sentence = ctrl_nlp_th->request_sentence;

// parsing the numbers
  double value = 0;
  int request_word_used = OgHeapGetCellsUsed(ctrl_nlp_th->hrequest_word);

  struct request_word *request_word_start;
  struct request_word *request_word_end;

  char pattern[DPcPathSize];
  snprintf(pattern, DPcPathSize, "^((?:(?:\\d{1,3}(?:%s\\d{3})+)|\\d+))(?:%s(\\d*))?$", g_strescape(thousand_sep, NULL), g_strescape(decimal_sep, NULL));

  GError *regexp_error = NULL;
  GRegex *regular_expression = g_regex_new(pattern, 0, 0, &regexp_error);

  if (request_word_used > 0)
  {
    request_word_start = OgHeapGetCell(ctrl_nlp_th->hrequest_word, 0);
    IFN(request_word_start) DPcErr;
  }
  else
  {
    DPcErr;
  }

  og_bool previous_match = FALSE;
  request_word_end = request_word_start;

  for (request_word_end = request_word_start;
      request_word_end && request_word_end->self_index < ctrl_nlp_th->basic_request_word_used; request_word_end =
          request_word_end->next)
  {
    og_string string_request_word = OgHeapGetCell(ctrl_nlp_th->hba, request_word_end->start);
    IFN(string_request_word) DPcErr;

    // on ne prend pas en compte les séparateurs en fin de nombre
    if (!strcmp(string_request_word, thousand_sep) || !strcmp(string_request_word, decimal_sep))
    {
      continue;
    }

    // recuperer la string entre DigitGroupStart et DigitGroupEnd
    og_char_buffer expression_string[DPcPathSize];
    int start_position = request_word_start->start_position;
    int end_position = request_word_end->start_position + request_word_end->length_position;
    int length = end_position - start_position;
    snprintf(expression_string, DPcPathSize, "%.*s", length, request_sentence + start_position);

    og_bool number_match = NlpNumberParsing(ctrl_nlp_th, expression_string, regular_expression, thousand_sep, &value);
    IFE(number_match);

    if (number_match)
    {
      if(request_word_start != request_word_end)
      {
        digits_grouped = TRUE;
      }
      request_word_start->next = request_word_end->next;
      request_word_start->length_position = request_word_end->start_position - request_word_start->start_position
          + request_word_end->length_position;
      request_word_start->is_digit = TRUE;
      request_word_start->digit_value = value;
      previous_match = TRUE;
    }
    else
    {
      if (previous_match)   // si on a 123.456 789 XXXX, on a un match à false, mais il faut reconsidérer 789 comme un nombre
      {
        // recuperer la string entre DigitGroupStart et DigitGroupEnd
        start_position = request_word_end->start_position;
        end_position = request_word_end->start_position + request_word_end->length_position;
        length = end_position - start_position;
        snprintf(expression_string, DPcPathSize, "%.*s", length, request_sentence + start_position);

        number_match = NlpNumberParsing(ctrl_nlp_th, expression_string, regular_expression, thousand_sep, &value);
        IFE(number_match);

        if (number_match)   // on est dans le cas 123.456 789 XXXX, il faut considérer le 789 comme un nombre
        {
          request_word_start = request_word_end;
          request_word_start->is_digit = TRUE;
          request_word_start->digit_value = value;
        }
        else   // on est dans le cas 123.456 aa XXXX, il faut recommencer le parsing au mot suivant
        {
          if (request_word_end->next)
          {
            request_word_start = request_word_end->next;
          }
          previous_match = FALSE;
        }

      }
      else
      {
        if (request_word_end->next)
        {
          request_word_start = request_word_end->next;
        }
      }
    }
  }

  g_regex_unref(regular_expression);
  return digits_grouped;
}

og_bool NlpMatchWordGroupDigits(og_nlp_th ctrl_nlp_th)
{
  og_bool digits_grouped = FALSE;
  og_bool language_found = FALSE;

  struct lang_sep* lang_conf = lang_sep_conf;
  struct accept_language* acceptedlanguage;
  struct lang_sep found_language;
  og_char_buffer thousand_sep[5];
  og_char_buffer decimal_sep[5];

  size_t numberLang = OgHeapGetCellsUsed(ctrl_nlp_th->haccept_language);

  if(numberLang > 0) // languages are given in the request
  {
    // check the given languages
    for (size_t i = 0; i < numberLang && !digits_grouped; i++)
    {
      acceptedlanguage = OgHeapGetCell(ctrl_nlp_th->haccept_language, i);
      language_found = getNumberSeparators(lang_conf, acceptedlanguage->locale, thousand_sep, decimal_sep);
      found_language.decimal_sep = decimal_sep;
      found_language.thousand_sep = thousand_sep;
      digits_grouped = NlpMatchWordGroupDigitsByLanguage(ctrl_nlp_th, &found_language);
      IFE(digits_grouped);
    }
  }
  if(numberLang <= 0 || language_found == FALSE) // no languages are given or given languages not found, let's check them all
  {
    for (struct lang_sep *conf = lang_conf; !digits_grouped && conf->lang != DOgLangNil && conf->country != DOgCountryNil; conf++)
    {
      digits_grouped = NlpMatchWordGroupDigitsByLanguage(ctrl_nlp_th, conf);
      IFE(digits_grouped);
    }
  }

  return digits_grouped;
}

og_status NlpMatchWordChainRequestWords(og_nlp_th ctrl_nlp_th)
{
  int request_word_used = OgHeapGetCellsUsed(ctrl_nlp_th->hrequest_word);
  struct request_word *all_words = OgHeapGetCell(ctrl_nlp_th->hrequest_word, 0);
  IFN(all_words) DPcErr;

  struct request_word *previous_word = NULL;

  for (int i = 0; i < request_word_used; i++)
  {
    struct request_word *current_word = all_words + i;
    current_word->next = NULL;

    if (previous_word)
    {
      previous_word->next = current_word;
    }

    previous_word = current_word;
  }
  DONE;
}

