<?php
/**
 * @file
 *
 * Newspaper Search form
 */

/**
 * Creates the custom search form for searching Manitoba's newspapers.
 *
 * @TODO: Limit searches based upon newspapers only.
 *
 * @param array $form
 *   The form to construct
 *
 * @param array $form_state
 *   Its state
 *
 * @return array
 *   The constructed form
 */
function manidora_newspaper_search_form($form, &$form_state) {
  module_load_include('inc', 'islandora_solr', 'includes/query_processor');
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $solr_build = new IslandoraSolrQueryProcessor();

  $solr_query = '+RELS_EXT_hasModel_uri_ms:"info:fedora/islandora:newspaperCModel"';

  $solr_build->buildQuery($solr_query);

  $solr_build->solrParams['facet'] = $solr_build->solrParams['hl'] = 'false';
  $solr_build->solrParams = islandora_solr_remove_base_filters($solr_build->solrParams);
  $solr_build->solrParams['sort'] = "fgs_label_s ASC";

  try {
    $solr_build->executeQuery(FALSE);
    $results = $solr_build->islandoraSolrResult["response"]["objects"];
  }
  catch (Exception $e) {
    $results = array();
    islandora_solr_technical_difficulties($e);
  }

  $all_years = array('min' => 10000, 'max' => 0);
  $tmpobjects = array();
  if ($results) {
    foreach ($results as $key => $doc) {
      if (strlen($doc['object_label']) > 0) {
        $tmpobjects[$doc['PID']] = $doc['object_label'];
        $sql = 'RELS_EXT_hasModel_uri_ms:"info:fedora/islandora:newspaperIssueCModel" AND ancestors_ms:"' . $doc['PID'] . '"';
        $sp = array(
          'facet' => 'false',
          'hl' => 'false',
          'rows' => 0,
          'stats' => 'true',
          'stats.field' => 'mods_originInfo_dateIssued_dt',
        );
        $solr_build->buildQuery($sql, $sp);
        $solr_build->solrParams = array_merge_recursive($solr_build->solrParams, $sp);
        $solr_build->solrParams = islandora_solr_remove_base_filters($solr_build->solrParams);
        try {
          $solr_build->executeQuery(FALSE);
          $years = $solr_build->islandoraSolrResult['stats']['stats_fields']['mods_originInfo_dateIssued_dt'];
        }
        catch (Exception $e) {
          $years = array();
          drupal_set_message(check_plain(t('Error searching Solr index')) . ' ' . $e->getMessage(), 'error', FALSE);
        }

        if (isset($years['min']) && isset($years['max'])) {
          $tmpobjects[$doc['PID']] .= ' (' . substr($years['min'], 0, 4) . '-' . substr($years['max'], 0, 4) . ')';
          if (substr($years['min'], 0, 4) < $all_years['min']) {
            $all_years['min'] = substr($years['min'], 0, 4);
          }
          if (substr($years['max'], 0, 4) > $all_years['max']) {
            $all_years['max'] = substr($years['max'], 0, 4);
          }
        }
      }
    }
  }

  uasort($tmpobjects, function($a ,$b) {
    return strcasecmp(
      preg_replace("/^the\s?/i", "", $a),
      preg_replace("/^the\s?/i", "", $b));
    }
  );

  $form = array(
    'wrapper' => array(
      '#type' => 'container',
      '#attributes' => array('class' => array('column-container')),
      'left_column' => array(
        '#type' => 'container',
        '#attributes' => array('class' => array('column-split-50')),
      ),
      'mid_column' => array(
        '#type' => 'container',
        '#attributes' => array('class' => array('column-split-25')),
      ),
      'right_column' => array(
        '#type' => 'container',
        '#attributes' => array('class' => array('column-split-25')),
      ),
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Search'),
      '#submit' => array('manidora_newspaper_search_form_submit'),
    ),
  );
  
  // Allows us to shorten the below lines
  $f =& $form['wrapper'];

  $f['left_column']['exact_phrase'] = array(
    '#type' => 'textfield',
    '#title' => t('With the exact phrase'),
    '#size' => 50,
  );
  $f['left_column']['least_one'] = array(
    '#type' => 'textfield',
    '#title' => t('With at least one of the words'),
    '#size' => 50,
  );
  $f['left_column']['all_words'] = array(
    '#type' => 'textfield',
    '#title' => t('With all of the words'),
    '#size' => 50,
  );
  $f['left_column']['without_words'] = array(
    '#type' => 'textfield',
    '#title' => t('Without these words'),
    '#suffix' => '<p class="without-words-suffix">' . t('If you are searching for "Winnipeg football" but want to exclude references to the Winnipeg Blue Bombers in your search results, you would enter the words "Blue Bombers" in this box.') . '</p>',
    '#size' => 50,
  );
  

  $f['mid_column']['titles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Limit to only these titles'),
    '#options' => $tmpobjects,
    // '#default_value' => array_keys($tmpobjects),
  );
  $f['mid_column']['select_all'] = array(
    '#type' => 'markup',
    '#markup' => '<span><a id="manidora-newspaper-search-select-all" href="javascript://">' . t('Select') . '</a></span>/<span ><a id="manidora-newspaper-search-select-none" href="javascript://">' . t('Deselect') . '</a> all titles</span>',
  );
  $f['mid_column']['browse'] = array(
    '#type' => 'markup',
    '#markup' => '<p>' . l(t('Browse all newspapers'), 'islandora/object/uofm:libraries') . '</p>',
  );
  $f['right_column']['date_text'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('date-limiter')),
    '#prefix' => '<p class="label">' . t('Limit to a date range') . '</p>',
    '#suffix' => '<p>Format (1900/05/23)</p>',
  );
  $f['right_column']['date_text']['date_from'] = array(
    '#type' => 'textfield',
    '#title' => t('from'),
    '#size' => 10,
    '#suffix' => '<span class="date-spacer">  ' . t('-') . '  </span>',
  );
  $f['right_column']['date_text']['date_to'] = array(
    '#type' => 'textfield',
    '#title' => t('to'),
    '#size' => 10,
  );
  if (isset($all_years) && count($all_years) > 0) {
    $f['right_column']['date_range_low'] = array(
      '#type' => 'hidden',
      '#value' => $all_years['min'],
    );
    $f['right_column']['date_range_high'] = array(
      '#type' => 'hidden',
      '#value' => $all_years['max'],
    );
  }
  $form['#attached']['js'][] = array(
    'type' => 'inline',
    'data' => 'jQuery(document).ready(function () { jQuery("#manidora-newspaper-search-select-all").click(function() { jQuery("#edit-titles input:checkbox").prop("checked","checked");}); jQuery("#manidora-newspaper-search-select-none").click(function() { jQuery("#edit-titles input:checkbox").removeAttr("checked");});});',
    'scope' => 'footer',
    'group' => JS_THEME,
    'weight' => 5,
  );

  drupal_set_title(t('Newspaper Search'));
  drupal_set_breadcrumb(array());

  return $form;
}

/**
 * Calls hook_form_validate for newspaper search.
 *
 * Validates date ranges if selected.
 *
 * @param array $form
 *   The form to submit
 * @param array $form_state
 *   Its state
 */
function manidora_newspaper_search_form_validate($form, &$form_state) {
  if (isset($form_state['values']['date_from']) && !empty($form_state['values']['date_from'])) {
    if (!preg_match('/^([0-9]{4})\/?([0-9]{2})?\/?([0-9]{2})?$/', $form_state['values']['date_from'], $bits)) {
      form_set_error('date_from', t('From date must be of the form "YYYY/MM/DD"'));
    }
    else {
      $from['year'] = $bits[1];
      $from['iso'] = $bits[1];
      if (count($bits) > 2) {
        $from['month'] = $bits[2];
        $from['iso'] .= $bits[2];
      }
      if (count($bits) > 3) {
        $from['day'] = $bits[3];
        $from['iso'] .= $bits[3];
      }
      if (array_key_exists('day', $from) && !checkdate($from['month'], $from['day'], $from['year'])) {
        form_set_error('date_from', t('From date was not valid, please ensure your date is of form "YYYY/MM/DD"'));
      }
    }
  }
  if (isset($form_state['values']['date_to']) && !empty($form_state['values']['date_to'])) {
    if (!preg_match('/^([0-9]{4})\/?([0-9]{2})?\/?([0-9]{2})?$/', $form_state['values']['date_to'], $bits)) {
      form_set_error('date_to', t('To date must be of the form "YYYY/MM/DD"'));
    }
    else {
      $to['year'] = $bits[1];
      $to['iso'] = $bits[1];
      if (count($bits) > 2) {
        $to['month'] = $bits[2];
        $to['iso'] .= $bits[2];
      }
      if (count($bits) > 3) {
        $to['day'] = $bits[3];
        $to['iso'] .= $bits[3];
      }
      if (array_key_exists('day', $to) && !checkdate($to['month'], $to['day'], $to['year'])) {
        form_set_error('date_to', t('To date was not valid, please ensure your date is of form "YYYY/MM/DD"'));
      }
    }
  }
  if (isset($from) && isset($to)) {
    if ($from['iso'] > $to['iso']) {
      form_set_error('date_from', t('From date cannot be larger than To date'));
    }
  }
  if (isset($form_state['values']['date_range_low'])) {
    if (isset($from) && $from['year'] > $form_state['values']['date_range_high']) {
      form_set_error('date_from', t('Our newspapers only go up to the year (%dt), please adjust your date range.', array('%dt' => $form_state['values']['date_range_high'])));

    }
    if (isset($to) && $to['year'] < $form_state['values']['date_range_low']) {
      form_set_error('date_to', t('Our newspapers only start at the year (%dt), please adjust your date range.', array('%dt' => $form_state['values']['date_range_low'])));
    }
  }
}

/**
 * Submit handler for the manidora_newspaper_search_form.
 *
 * Redirects to the islandora/solr/search menupath to execute the
 * given query.
 *
 * @param array $form
 *   The form to submit
 * @param array $form_state
 *   Its state
 */
function manidora_newspaper_search_form_submit($form, &$form_state) {
  global $_islandora_solr_queryclass;
  $query_string = '';
  $params = isset($_islandora_solr_queryclass->internalSolrParams) ? $_islandora_solr_queryclass->internalSolrParams : array();
  if (strlen($form_state['values']['all_words']) > 0) {
    $all_words = $form_state['values']['all_words'];
    $all_array = explode(' ', $all_words);

    $count = 0;
    foreach ($all_array as $word) {
      if ($count == 0) {
        $query_string .= '(' . $word;
      }
      else {
        $query_string .= ' AND ' . $word;
      }
      $count++;
    }
    $query_string .= ')';
  }

  if (strlen($form_state['values']['exact_phrase']) > 0) {
    $exact_phrase = $form_state['values']['exact_phrase'];
    $exact_phrase = '"' . $exact_phrase . '"';

    if (!strlen($query_string)) {
      $query_string .= '(' . $exact_phrase . ')';
    }
    else {
      $query_string .= ' AND (' . $exact_phrase . ')';
    }
  }

  // "x" or field:"y" or field:"z"
  if (strlen($form_state['values']['least_one']) > 0) {
    $least_one = $form_state['values']['least_one'];
    $least_array = explode(' ', $least_one);
    $count = 0;
    if (strlen($query_string)) {
      $query_string .= ' AND ';
    }
    foreach ($least_array as $word) {
      if ($count == 0) {
        $query_string .= '(' . $word;
      }
      else {
        $query_string .= ' OR ' . $word;
      }
      $count++;
    }
    $query_string .= ')';
  }

  // Negative of "with all of the words"
  if (strlen($form_state['values']['without_words']) > 0) {
    $without_words = $form_state['values']['without_words'];
    $without_array = explode(' ', $without_words);
    $count = 0;
    if (strlen($query_string)) {
      $query_string .= ' AND ';
    }
    foreach ($without_array as $word) {
      if ($count == 0) {
        $query_string .= '-(' . $word;
      }
      else {
        $query_string .= ' AND ' . $word;
      }
      $count++;
    }
    $query_string .= ')';
  }
  /* Date limiting */

  $iso_date = function($date) {
    $date_array = explode('/', $date);
    if (!isset($date_array[1])) {
      $date_array[1] = '01';
    }
    if (!isset($date_array[2])) {
      $date_array[2] = '01';
    }
    return vsprintf('%s-%s-%sT00:00:00Z', $date_array);
  };

  if (strlen($form_state['values']['date_from']) > 0) {
    $from = $iso_date($form_state['values']['date_from']);
  }

  if (strlen($form_state['values']['date_to']) > 0) {
    $to = $iso_date($form_state['values']['date_to']);
  }
  if (isset($from) || isset($to)) {
    if (!isset($from)) {
      $from = $form_state['values']['date_range_low'] . '-01-01T00:00:00Z';
    }
    if (!isset($to)) {
      $to = $form_state['values']['date_range_high'] . '-01-01T00:00:00Z';
    }
    $date_limit = 'mods_originInfo_dateIssued_dt:[' . $from . ' TO ' . $to . ']';
  }

  // Did we select at least 1, but not all newspapers?
  $selected_papers = array();
  foreach ($form_state['values']['titles'] as $key => $val) {
    if ($key === $val) {
      $selected_papers[] = $key;
    }
  }

  $hidden_filter = array('RELS_EXT_hasModel_uri_ms:("info:fedora/islandora:newspaperPageCModel" OR "info:fedora/islandora:newspaperCModel" OR "info:fedora/islandora:newspaperIssueCModel")');
  if (count($selected_papers) > 0 && count($selected_papers) < count($form_state['values']['titles'])) {
    $hidden_filter = array('ancestors_ms:("' . implode('" OR "', $selected_papers) . '")');
    // To show the facet.
    /* $query['f'] = array('ancestors_ms:("'.implode('" OR "', $selected_papers).'")'); */
  }
  $query_string = str_replace('/', '~slsh~', $query_string);
  /* $query = array(
  'type' => 'edismax',
  'hidden_filter' => $hidden_filter,
  );*/
  $params['type'] = 'edismax';
  $params['hidden_filter'] = $hidden_filter;
  // Set date filter key if there are no date filters included.
  if (isset($date_limit)) {
    $query = array_merge_recursive($params, array('f' => array($date_limit)));
  }
  else {
    $query = $params;
  }

  drupal_goto('islandora/search/' . $query_string, array(
    'query' => $query,
  ));
}
