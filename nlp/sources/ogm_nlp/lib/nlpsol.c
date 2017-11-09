/*
 *  Calculate solutions for the expressions tree.
 *  Copyright (c) 2017 Pertimm, by Patrick Constant
 *  Dev : October 2017
 *  Version 1.0
 */
#include "ogm_nlp.h"

static og_bool NlpSolutionCalculateRecursive(og_nlp_th ctrl_nlp_th, struct request_expression *root_request_expression,
    struct request_expression *request_expression);
static og_bool NlpSolutionAdd(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression);
static og_bool NlpSolutionCombine(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression);
static og_bool NlpSolutionBuildSolutionsQueue(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression);
static og_status NlpSolutionMergeObjects(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression);
static og_status NlpSolutionMergeObjectsRecursive(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression, GList *iter_solutions);
static og_bool NlpSolutionComputeJS(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression,
    json_t *json_package_solution);

og_status NlpSolutionCalculate(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression)
{
  IFE(NlpSolutionCalculateRecursive(ctrl_nlp_th, request_expression, request_expression));
  DONE;
}

static og_bool NlpSolutionCalculateRecursive(og_nlp_th ctrl_nlp_th, struct request_expression *root_request_expression,
    struct request_expression *request_expression)
{
  og_bool must_combine_solution = FALSE;

  for (int i = 0; i < request_expression->orips_nb; i++)
  {
    struct request_input_part *request_input_part = NlpGetRequestInputPart(ctrl_nlp_th, request_expression, i);
    IFN(request_input_part) DPcErr;

    if (request_input_part->type == nlp_input_part_type_Word)
    {
      struct request_word *request_word = request_input_part->request_word;
      og_string string_request_word = OgHeapGetCell(ctrl_nlp_th->hba, request_word->start);
      IFN(string_request_word) DPcErr;
    }
    else if (request_input_part->type == nlp_input_part_type_Interpretation)
    {
      struct request_expression *sub_request_expression = OgHeapGetCell(ctrl_nlp_th->hrequest_expression,
          request_input_part->Irequest_expression);
      IFN(sub_request_expression) DPcErr;
      og_bool sub_solution_built = NlpSolutionCalculateRecursive(ctrl_nlp_th, root_request_expression,
          sub_request_expression);
      IFE(sub_solution_built);
      if (sub_solution_built) must_combine_solution = TRUE;
    }

    if (request_expression->expression->alias_any_input_part_position == i + 1)
    {
      if (request_expression->Irequest_any >= 0)
      {
        must_combine_solution = TRUE;
      }
    }
  }

  og_bool solution_built = FALSE;

  if (must_combine_solution)
  {
    json_t *json_package_solution = request_expression->expression->json_solution;
    if (json_package_solution == NULL)
    {
      json_package_solution = request_expression->expression->interpretation->json_solution;
    }
    if (json_package_solution != NULL)
    {
      IFE(solution_built = NlpSolutionComputeJS(ctrl_nlp_th, request_expression, json_package_solution));
    }
    else
    {
      IFE(solution_built = NlpSolutionCombine(ctrl_nlp_th, request_expression));
    }
  }
  else
  {
    IFE(solution_built = NlpSolutionAdd(ctrl_nlp_th, request_expression));
  }

  return solution_built;
}

og_status NlpRequestSolutionString(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression, int size,
    char *string)
{
  if (request_expression->json_solution)
  {
    return NlpJsonToBuffer(request_expression->json_solution, string, size, NULL, 0);
  }
  else
  {
    string[0] = 0;
  }
  DONE;
}

og_status NlpSolutionString(og_nlp_th ctrl_nlp_th, json_t *json_solution, int size, char *string)
{
  if (json_solution)
  {
    return NlpJsonToBuffer(json_solution, string, size, NULL, 0);
  }
  else
  {
    string[0] = 0;
  }
  DONE;
}

static og_bool NlpSolutionAdd(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression)
{
  og_bool solution_built = FALSE;
  if (request_expression->expression->json_solution != NULL)
  {
    request_expression->json_solution = json_deep_copy(request_expression->expression->json_solution);
    solution_built = TRUE;
  }
  else if (request_expression->expression->interpretation->json_solution != NULL)
  {
    request_expression->json_solution = json_deep_copy(request_expression->expression->interpretation->json_solution);
    solution_built = TRUE;
  }
  return solution_built;
}

static og_bool NlpSolutionCombine(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression)
{
  GQueue *solutions = request_expression->tmp_solutions;
  IFE(NlpSolutionBuildSolutionsQueue(ctrl_nlp_th, request_expression));

  if (ctrl_nlp_th->loginfo->trace & DOgNlpTraceSolution)
  {
    NlpLog(DOgNlpTraceSolution, "NlpSolutionCombine: list of solutions for expression:");
    IFE(NlpRequestExpressionLog(ctrl_nlp_th, request_expression, 0));
    for (GList *iter = solutions->head; iter; iter = iter->next)
    {
      struct alias_solution *alias_solution = iter->data;
      json_t *json_solution = alias_solution->json_solution;
      og_char_buffer json_solution_string[DOgMlogMaxMessageSize / 2];
      IFE(NlpJsonToBuffer(json_solution, json_solution_string, DOgMlogMaxMessageSize / 2, NULL, 0));
      NlpLog(DOgNlpTraceSolution, "  solution: %s", json_solution_string);
    }
  }

  int nb_solutions = g_queue_get_length(solutions);
  if (nb_solutions <= 0) return FALSE;
  else if (nb_solutions == 1)
  {
    struct alias_solution *alias_solution = solutions->head->data;
    request_expression->json_solution = json_incref(alias_solution->json_solution);
  }
  else
  {
    int nb_objects = 0;
    int nb_arrays = 0;
    for (GList *iter = solutions->head; iter; iter = iter->next)
    {
      struct alias_solution *alias_solution = iter->data;
      json_t *json_sub_solution = alias_solution->json_solution;
      if (json_is_object(json_sub_solution)) nb_objects++;
      else if (json_is_array(json_sub_solution)) nb_arrays++;
      else
      {
        og_char_buffer json_sub_solution_string[DOgMlogMaxMessageSize / 2];
        IFE(NlpJsonToBuffer(json_sub_solution, json_sub_solution_string, DOgMlogMaxMessageSize / 2, NULL, 0));
        NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionCombine: json_sub_solution should be array or object : %s",
            json_sub_solution_string);
        DPcErr;
      }
    }

    if (nb_arrays <= 0)
    {
      // Combining the objects into a single object
      request_expression->json_solution = json_object();
      IFE(NlpSolutionMergeObjects(ctrl_nlp_th, request_expression));
    }
    else
    {
      //TODO: append array and add objects into newly formed array
    }
  }
  // g_queue_clear(solutions);

  return TRUE;
}

static og_bool NlpSolutionBuildSolutionsQueue(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression)
{
  int alias_solutions_nb = 0;

  for (int i = 0; i < request_expression->orips_nb; i++)
  {
    struct request_input_part *request_input_part = NlpGetRequestInputPart(ctrl_nlp_th, request_expression, i);
    IFN(request_input_part) DPcErr;

    if (request_input_part->type == nlp_input_part_type_Interpretation)
    {
      struct request_expression *sub_request_expression = OgHeapGetCell(ctrl_nlp_th->hrequest_expression,
          request_input_part->Irequest_expression);
      IFN(sub_request_expression) DPcErr;
      IFX(sub_request_expression->json_solution)
      {
        struct alias *alias = request_expression->expression->aliases + i;
        char solution[DPcPathSize];
        NlpSolutionString(ctrl_nlp_th, sub_request_expression->json_solution, DPcPathSize, solution);
        NlpLog(DOgNlpTraceSolution, "NlpSolutionBuildSolutionsQueue: for alias '%s' building solution : %s",
            alias->alias, solution);
        if (alias_solutions_nb >= DOgAliasSolutionSize)
        {
          NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionCombine: alias_solutions_nb (%d) >= DOgAliasSolutionSize (%d)",
              alias_solutions_nb, DOgAliasSolutionSize);
          DPcErr;

        }
        struct alias_solution *alias_solution = g_slice_new0(struct alias_solution);
        alias_solution->alias = alias;
        alias_solution->json_solution = json_incref(sub_request_expression->json_solution);

        g_queue_push_tail(request_expression->tmp_solutions, alias_solution);
        alias_solutions_nb++;
      }
    }
    if (request_expression->expression->alias_any_input_part_position == i + 1)
    {
      struct request_any *request_any = OgHeapGetCell(ctrl_nlp_th->hrequest_any, request_expression->Irequest_any);
      IFN(request_any) DPcErr;

      char string_any[DPcPathSize];
      NlpRequestAnyString(ctrl_nlp_th, request_any, DPcPathSize, string_any);

      json_t *json_solution_any = json_object();
      json_t *json_solution_string_any = json_string(string_any);
      IF(json_object_set_new(json_solution_any, "any", json_solution_string_any))
      {
        NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionCombine: error setting json_sub_expression_any");
        DPcErr;
      }

      struct alias *alias = request_expression->expression->aliases
          + request_expression->expression->alias_any_input_part_position;
      char solution[DPcPathSize];
      NlpSolutionString(ctrl_nlp_th, json_solution_any, DPcPathSize, solution);
      NlpLog(DOgNlpTraceSolution, "NlpSolutionBuildSolutionsQueue: for alias '%s' building solution : %s", alias->alias,
          solution);
      if (alias_solutions_nb >= DOgAliasSolutionSize)
      {
        NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionCombine: alias_solutions_nb (%d) >= DOgAliasSolutionSize (%d)",
            alias_solutions_nb, DOgAliasSolutionSize);
        DPcErr;

      }
      struct alias_solution *alias_solution = g_slice_new0(struct alias_solution);
      alias_solution->alias = alias;
      alias_solution->json_solution = json_solution_any;
      g_queue_push_tail(request_expression->tmp_solutions, alias_solution);
      alias_solutions_nb++;
    }
  }
  DONE;
}

static og_status NlpSolutionMergeObjects(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression)
{
  GList *iter_solutions = request_expression->tmp_solutions->head;
  return NlpSolutionMergeObjectsRecursive(ctrl_nlp_th, request_expression, iter_solutions);
}

static og_status NlpSolutionMergeObjectsRecursive(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression, GList *iter_solutions)
{
  IFN(iter_solutions) DONE;
  struct alias_solution *alias_solution_first = iter_solutions->data;
  json_t *json_solution_first = alias_solution_first->json_solution;
  const char *key;
  json_t *value;
  json_object_foreach(json_solution_first, key, value)
  {
    if (json_object_get(request_expression->json_solution, key)) continue;
    og_bool several_values = FALSE;
    for (GList *iter = iter_solutions->next; iter; iter = iter->next)
    {
      struct alias_solution *alias_solution = iter->data;
      json_t *json_sub_solution = alias_solution->json_solution;
      json_t *sub_value = json_object_get(json_sub_solution, key);
      if (sub_value)
      {
        several_values = TRUE;
        break;
      }
    }
    if (several_values)
    {
      json_t *json_array_values = json_array();
      IF(json_object_set_new(request_expression->json_solution, key, json_array_values))
      {
        NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionMergeObjectsRecursive: error setting key '%s'", key);
        DPcErr;
      }

      for (GList *iter = iter_solutions; iter; iter = iter->next)
      {
        struct alias_solution *alias_solution = iter->data;
        json_t *json_sub_solution = alias_solution->json_solution;
        json_t *sub_value = json_object_get(json_sub_solution, key);
        IFN(sub_value) continue;
        IF(json_array_append(json_array_values, sub_value))
        {
          NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionMergeObjectsRecursive : Error while adding json_array_values");
          DPcErr;
        }
      }

    }
    else
    {
      IF(json_object_set(request_expression->json_solution, key, value))
      {
        NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionMergeObjectsRecursive: error setting key '%s'", key);
        DPcErr;
      }
    }
  }

  return NlpSolutionMergeObjectsRecursive(ctrl_nlp_th, request_expression, iter_solutions->next);

}

static og_bool NlpSolutionComputeJS(og_nlp_th ctrl_nlp_th, struct request_expression *request_expression,
    json_t *json_package_solution)
{
  og_bool solution_built = FALSE;
  IFE(NlpSolutionBuildSolutionsQueue(ctrl_nlp_th, request_expression));

  if (ctrl_nlp_th->loginfo->trace & DOgNlpTraceSolution)
  {
    char solution[DPcPathSize];
    NlpSolutionString(ctrl_nlp_th, json_package_solution, DPcPathSize, solution);
    NlpLog(DOgNlpTraceSolution, "NlpSolutionComputeJS: working on solution %s:", solution)

    NlpLog(DOgNlpTraceSolution, "NlpSolutionComputeJS: list of solutions for expression:");
    IFE(NlpRequestExpressionLog(ctrl_nlp_th, request_expression, 0));
    for (GList *iter = request_expression->tmp_solutions->head; iter; iter = iter->next)
    {
      struct alias_solution *alias_solution = iter->data;
      json_t *json_solution = alias_solution->json_solution;
      og_char_buffer json_solution_string[DOgMlogMaxMessageSize / 2];
      IFE(NlpJsonToBuffer(json_solution, json_solution_string, DOgMlogMaxMessageSize / 2, NULL, 0));
      NlpLog(DOgNlpTraceSolution, "  alias '%s' solution: %s", alias_solution->alias->alias, json_solution_string);
    }
  }

  // We want to add the alias as a variable whose name is the alias and whose value is its associated solution
  for (GList *iter = request_expression->tmp_solutions->head; iter; iter = iter->next)
  {
    struct alias_solution *alias_solution = iter->data;
    json_t *json_solution = alias_solution->json_solution;
    og_char_buffer json_solution_string[DOgMlogMaxMessageSize / 2];
    IFE(NlpJsonToBuffer(json_solution, json_solution_string, DOgMlogMaxMessageSize / 2, NULL, 0));
    og_char_buffer var_command[DOgMlogMaxMessageSize];
    snprintf(var_command, DOgMlogMaxMessageSize, "var %s = %s;", alias_solution->alias->alias, json_solution_string);
    NlpLog(DOgNlpTraceSolution, "Sending variable to duktape: %s", var_command);
    if (duk_peval_lstring(ctrl_nlp_th->duk_context, var_command, strlen(var_command)) != 0)
    {
      NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionComputeJS: duk_peval_lstring eval failed: %s",
          duk_safe_to_string(ctrl_nlp_th->duk_context, -1));
      DPcErr;
    }
  }

  NlpLog(DOgNlpTraceSolution, "Transforming solution by executing JS code");
  // Now we scan the json_package_solution to get some executable code
  // If there are, we create a copy of the json structure and amend it with the result of the execution of the code
  json_t *json_new_solution = json_deep_copy(json_package_solution);

  for (void *iter = json_object_iter(json_new_solution); iter; iter = json_object_iter_next(json_new_solution, iter))
  {
    json_t *value = json_object_iter_value(iter);
    if (json_is_string(value))
    {
      og_string string_value = json_string_value(value);
      int string_value_length = strlen(string_value);
      if (string_value[0] == '`' && string_value[string_value_length - 1] == '`')
      {
        NlpLog(DOgNlpTraceSolution, "string to evaluate : '%.*s'", string_value_length - 2, string_value + 1);
        if (duk_peval_lstring(ctrl_nlp_th->duk_context, string_value + 1, string_value_length - 2) != 0)
        {
          NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionComputeJS: duk_peval_lstring eval failed: %s",
              duk_safe_to_string(ctrl_nlp_th->duk_context, -1));
          DPcErr;
        }
        duk_int_t type = duk_get_type(ctrl_nlp_th->duk_context, -1);
        NlpLog(DOgNlpTraceSolution, "type is %s", NlpDukTypeString(type));
        if (type == DUK_TYPE_STRING)
        {
          og_string computed_string_value = duk_get_string(ctrl_nlp_th->duk_context, -1);
          NlpLog(DOgNlpTraceSolution, "computed value is '%s'", computed_string_value);

          json_t *json_computed_value = json_string(computed_string_value);
          IF(json_object_iter_set_new(json_new_solution, iter, json_computed_value))
          {
            NlpThrowErrorTh(ctrl_nlp_th, "NlpSolutionComputeJS : Error creating json_computed_value '%s'",
                computed_string_value);
            DPcErr;
          }
          solution_built = TRUE;
        }
      }
    }
  }

  if (solution_built)
  {
    request_expression->json_solution = json_new_solution;
  }
  else
  {
    json_decref(json_new_solution);
  }
  return (solution_built);
}
