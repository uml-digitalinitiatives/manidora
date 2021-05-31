<?php


/**
 * @file
 * Contains functions for blocks.
 */

 /**
  * Implements hook_block_info().
  */
function manidora_block_info() {
  $blocks['manidora_homepage_tabs'] = array(
    'info' => t('Manidora homepage tabs'),
    'cache' => DRUPAL_CACHE_PER_USER,
  );
  $blocks['manidora_homepage_thumbnails'] = array(
    'info' => t('Manidora homepage thumbnails'),
    'cache' => DRUPAL_CACHE_GLOBAL,
  );
  $blocks['manidora_whats_new'] = array(
    'info' => t("Manidora what's new"),
    'cache' => DRUPAL_CACHE_PER_USER,
  );
  $blocks['manidora_preview_image'] = array(
    'info' => t('Manidora preview thumbnail'),
    'cache' => DRUPAL_NO_CACHE,
  );
  return $blocks;
}

/**
 * Implements hook_block_view().
 */
function manidora_block_view($delta = '') {
  $block = array();
  switch ($delta) {
    case 'manidora_homepage_tabs':
      $block['subject'] = 'Homepage Tabs';
      $block['content'] = __manidora_homepage_tabs_content();
      break;

    case 'manidora_homepage_thumbnails':
      $block['subject'] = 'Homepage Thumbnails';
      $block['content'] = __manidora_homepage_thumbnail_content();
      break;

    case 'manidora_whats_new':
      $block['subject'] = 'What\'s New';
      $block['content'] = __manidora_whats_new_content();
      break;

    case 'manidora_preview_image':
      $block['subject'] = "Preview";
      $block['content'] = __manidora_preview_image();
      break;

  }
  return $block;
}

/**
 * Creates the homepage tabs content.
 *
 * @todo Ideally the content for the Fedora derived tabs (departments,
 * subjects, format) should be stored in a tab and administered through
 * Drupal. Unfortunately, due to time-constraints we are hard-coding.
 */
function __manidora_homepage_tabs_content() {

  $subjects = array(
    "Aboriginal Peoples",
    "Arts, Music, and Architecture",
    "Books, Literature, and Language",
    "Business, Industry, and Labour",
    "Education History",
    "Engineering and Transportation",
    "Government, Law, and Human Rights",
    "Health and Medicine",
    "Immigration",
    "Media and Communications",
    "Northern Studies",
    "Prairie History",
    "Religion, Philosophy, and Spiritualism",
    "Science and Technology",
    "Sports and Recreation",
    "University of Manitoba History",
    "War and the Military",
    "Women and Gender Studies",
    "World History, Peoples, and Cultures",
  );

  $formats = array(
    "Images" => array(
      "solr_field" => "type_of_resource_facet_ms",
      "data" => array(
        "Glass lantern slide",
        "Photograph",
        "Slide",
        "Stereo photograph",
        "Still image",
      ),
    ),
    "Text" => array(
      "solr_field" => "type_of_resource_facet_ms",
      "data" => array(
        "Text",
        "Textual record",
      ),
    ),
    "Newspapers" => array(
     /* "solr_field" => "type_of_resource_mt",
      "data" => array(
        "newspaper",
      ),*/
      "url" => "islandora/object/uofm:libraries",
    ),
    "Books" => array(
      "solr_field" => "RELS_EXT_hasModel_uri_ms",
      "data" => array(
        "info:fedora%2Fislandora:bookCModel",
      ),
    ),
    "Moving images" => array(
      "solr_field" => "type_of_resource_facet_ms",
      // "solr_field" => "dc.type",
      "data" => array(
        "Moving image",
      ),
    ),
    "Sound recordings" => array(
      "solr_field" => "type_of_resource_facet_ms",
      "data" => array(
        "Sound recording",
        "Sound recording - nonmusical",
      ),
    ),
    "Mixed material" => array(
      "solr_field" => "type_of_resource_facet_ms",
      "data" => array(
        "Mixed material",
      ),
    ),
  );

  $departments = array(
    // "Faculty of Architecture" => "",
    // "Faculty of Dentistry" => "uofm:dentalDAMS",
    "Faculty of Law" => "uofm:faclaw",
    "Faculty of Medicine Archives" => "uofm:medArchives",
    "Libraries: Architecture/Fine Arts Library" => "uofm:archfa",
    "Libraries: Archives & Special Collections" => "uofm:archives",
    "Libraries: Icelandic Collection" => "uofm:icelandic",
    "Libraries: Newspapers" => "uofm:libraries",
    "Restorative Dentistry Collection" => "uofm:restorativedentistry",
    "UM Marketing Communications Office" => "uofm:marketing",
    "UM Office of the Secretary" => "uofm:umsecretary",
    "UM Press" => "uofm:umpress",
  );

  $output = <<<EOF
  <ul class="home-tabs-container">
  <li class="home-tab active"><a title="List Collections Alphabetically" id="home-tabs-collections" class="home-tab-link" href="javascript:void(0)">Collections</a></li>
  <li class="home-tab"><a title="List Collections by Subject" id="home-tabs-subjects" class="home-tab-link" href="javascript:void(0)">Subjects</a></li>
  <li class="home-tab"><a title="Browse by Format type" id="home-tabs-formats" class="home-tab-link" href="javascript:void(0)">Formats</a></li>
  <li class="home-tab"><a title="List Collections by UM Faculty or Department" id="home-tabs-depts" class="home-tab-link" href="javascript:void(0)">Departments</a></li>
  </ul>
  <div class="home-panel" id="home-panel-subjects">
    <div class="column_50 first">
EOF;
  $half = intval(count($subjects) / 2) + (count($subjects) % 2);
  $count = 0;
  foreach ($subjects as $sub) {
    if ($count == $half) {
      $output .= "</div><!-- /.column_50 -->\n";
      $output .= "<div class=\"column_50 last\">\n";
    }
    $output .= l($sub, 'islandora/search/subject_topic_facet_ms:"' . $sub . '"') . "<br />\n";
    $count += 1;
  }
  $output .= <<<EOF
      </div><!-- /#.column_50 -->
    </div><!-- /#home-panel-subjects -->
    <div class="home-panel active" id="home-panel-collections">
EOF;

  $solr_build = new IslandoraSolrQueryProcessor();
  $solr_query = '(RELS_EXT_isMemberOfCollection_uri_mt:uofm* AND RELS_EXT_hasModel_uri_ms:("info:fedora/islandora:collectionCModel" OR "info:fedora/islandora:newspaperCModel"))';
  $expand_combine_pid = function(&$pid) {
    $pid = format_string('"info:fedora/!pid" OR "!pid"', ['!pid' => $pid]);
  };

  $excluded_pids = manidora_collection_get_excluded();
  array_walk($excluded_pids, $expand_combine_pid);
  $excluded_children = manidora_collection_get_excluded(True);
  array_walk($excluded_children, $expand_combine_pid);
  $solr_params = array(
    "fl" => "PID,dc.title,fgs_label_s",
    'rows' => 100000,
    'limit' => 100000,
    'facet' => 'false',
    'hl' => 'false',
  );
  if (count($excluded_pids) > 0 || count($excluded_children) > 0) {
    $solr_params['fq'] = [];
    if (count($excluded_pids) > 0) {
      $solr_params['fq'][] = format_string("-PID:(!excluded)", ['!excluded' => implode(' OR ', $excluded_pids)]);
    }
    if (count($excluded_children) > 0) {
      $solr_params['fq'][] = format_string("-!ancestor_field:(!ancestors)", [
        '!ancestor_field' => variable_get('manidora_omit_collection_ancestor_field', 'RELS_EXT_isMemberOfCollection_uri_ms'),
        '!ancestors' => implode(' OR ', $excluded_children)
      ]);
    }
  }
  $solr_build->buildQuery($solr_query, $solr_params);
  $solr_build->solrParams = array_merge_recursive($solr_build->solrParams, $solr_params);
  // We don't need faceting or highlighting here.
  $solr_build->solrParams['facet'] = $solr_build->solrParams['hl'] = 'false';

  // Query is executed.
  $solr_build->executeQuery(FALSE);
  $solr_results = (array) $solr_build->islandoraSolrResult['response']['objects'];

  if ($solr_results) {
    $get_nested = function($o, $name) {
      if (isset($o[$name])) {
        $tmp = $o[$name];
      }
      else if (isset($o['solr_doc'][$name])) {
        $tmp = $o['solr_doc'][$name];
      }
      if (isset($tmp)) {
        if (is_array($tmp)) {
          return reset($tmp);
        }
        else {
          return $tmp;
        }
      }
      return "";
    };

    $clean = function($o) use ($get_nested) {
      return array(
        'PID' => $get_nested($o, 'PID'),
        'title' => $get_nested($o, 'dc.title'),
      );
    };

    $tmpobjects = array_map($clean, $solr_results);

    // Solr can't sort by multivalued elements, so sort by title.
    usort($tmpobjects, '_manidora_internal_titlesort');
    $objects = array();
    $cl = "";
    /* Move the items so each letter that exists is an array of
    those collections. */
    array_walk($tmpobjects, function($v, $k) use (&$objects, &$cl) {
      if (strpos(strtolower($v['title']), "the ") === 0) {
        // Move "The" to the end
        $v['title'] = preg_replace('/^the\s?/i', "", $v['title']) . ', The';
      }
      if (substr(strtolower($v['title']), 0, 1) != $cl) {
        $cl = substr(strtolower($v['title']), 0, 1);
      }
      if (array_key_exists($cl, $objects) && !is_array($objects[$cl])) {
        $objects[$cl] = array();
      }
      $objects[$cl][] = $v;
    });
    $alpha_bar = "<span class=\"coll_alpha_list\">";
    $tmp = "<div class=\"manidora-collection-scroll\">";
    for ($letter_code = 97; $letter_code < (97+26); $letter_code += 1) {
      $letter = chr($letter_code);
      if (isset($objects[$letter])) {
        $alpha_bar .= "<a href=\"#collection_$letter\"> $letter </a>|";
        $collections = $objects[$letter];
        $odd = array();
        $even = array();
        $both = array(&$even, &$odd);
        /* Split items to even and odd array :
        http://stackoverflow.com/a/12405336 */
        array_walk($collections, function($v, $k) use ($both) {
          $both[$k % 2][] = $v;
        });
        $both = array_merge($even, $odd);
        $tmp .= "<div class=\"coll_letter_box\"><a name=\"collection_$letter\" id=\"collection_$letter\" class=\"collection_spacer\"></a>";
        $tmp .= "<div class=\"column_50 first collections\">";
        $count = 0;
        foreach ($both as $c) {
          if ($count == count($even)) {
            $tmp .= "</div><!-- /.column_50 -->\n";
            $tmp .= "<div class=\"column_50 last\">\n";
          }
          $tmp .= l($c['title'], 'islandora/object/' . $c['PID']) . "<br />\n";
          $count += 1;
        }
        $tmp .= "</div> <!-- /.column_50 -->\n";
        $tmp .= "</div> <!-- /.coll_letter_box -->\n";
      }
      else {
        $alpha_bar .= ' ' . $letter . " |";
      }
    }
    $tmp .= "</div> <!-- /.manidora-collection-scroll-box-->\n";
    $alpha_bar = substr($alpha_bar, 0, -2) . "</span>";

    $output .= $alpha_bar . $tmp;
  }

  $output .= <<<EOF
    </div><!-- /#home-panel-collections -->
    <div class="home-panel" id="home-panel-formats">
      <div class="column_50 first">
EOF;
  foreach ($formats as $key => $format) {
    $uri = "";
    if (array_key_exists('data', $format) && is_array($format['data']) && count($format['data']) > 0) {
      foreach ($format['data'] as $ele) {
        $uri .= (!empty($uri) ? " OR " : "") . '"' . $ele . '"';
      }
      $uri = $format['solr_field'] . ':(' . $uri . ')';
      if (!empty($uri)) {
        $uri = l($key, "islandora/search/$uri", array('html' => TRUE));
      }
    }
    elseif (array_key_exists('url', $format)) {
      $uri = l($key, $format['url']);
    }
    if (!empty($uri)) {
      $output .= "$uri<br />\n";
    }
    else {
      $output .= "$key<br />\n";
    }
  }
  $output .= <<<EOF
      </div><!-- /.column_50 -->
    </div><!-- /#home-panel-formats -->
    <div class="home-panel" id="home-panel-depts">
      <div class="column_50 first">
EOF;
  foreach ($departments as $dept => $uri) {
    if (!empty($uri)) {
      $output .= l($dept, "islandora/object/$uri") . "<br />\n";
    }
    else {
      $output .= "$dept<br />\n";
    }
  }
  $output .= <<<EOF
      </div><!-- /.column_50 -->
    </div><!-- /#home-panel-depts -->
    <script type="text/javascript">
    <!--
    jQuery(document).ready(function(){
      jQuery('.home-tab-link').click(function(){
        jQuery('.home-tab, .home-panel').removeClass('active');
        jQuery('#home-panel-' + jQuery(this).attr('id').replace('home-tabs-','')).addClass('active');
        jQuery(this).parent('.home-tab').addClass('active');
      });
      jQuery('a[href^="#"]', '#home-panel-collections').click(function(e) {
        e.preventDefault();
        var href = jQuery(this).attr('href');
        var target = jQuery(href, '#home-panel-collections');
        target.scrollParent().scrollTop(target.scrollParent().scrollTop() + target.position().top);
      });
    });
    //-->
    </script>
EOF;

  return $output;

}

/**
 * Sort two titles disregarding the leading word "the"
 * @param $a array first title
 * @param $b array second title
 * @return int < 0 if $a < $b, > 1 if $a > $b or 0 if $a == $b
 */
function _manidora_internal_titlesort($a, $b) {
  return strcasecmp(preg_replace("/^the\s?/i", "", $a["title"]),preg_replace("/^the\s?/i", "", $b["title"]));
}

/**
 * Display 16 thumbnails on the homepage from those selected.
 */
function __manidora_homepage_thumbnail_content() {
  //module_load_include('php', 'islandora_solr', 'SolrPhpClient/Apache/Solr/Service');
  module_load_include('inc', 'islandora_solr', 'includes/utilities');

  // How many thumbnails are we displaying?
  $num_pics = 16;
  // Get the pids from the variable.
  $thumbs = variable_get('manidora_thumbnail_pids');
  $output = "";
  if (!is_null($thumbs)) {
    // If its null we display nothing.
    $t_array = explode('~', urldecode($thumbs));
    /* This is temporary for debugging, we extend the array by
    duplicating it to see the full selection. */
    if (count($t_array) < $num_pics) {
      $dup = intval($num_pics / count($t_array)) + 1;
      $new_arr = array();
      for ($foo = 0; $foo < $dup; $foo += 1) {
        $new_arr = array_merge($new_arr, $t_array);
      }
      $t_array = $new_arr;
    }
    /* End temporary extension. */
    // We want to have the image title as mouseover in the block.
    // Search for all the possible PIDs.
    $solr_query = '+PID:("' . implode('" OR "', $t_array) . '")';
    // Get the PID and dc.title.
    $solr_params = array('fl' => 'PID,title_sort_ms');
    $solr_build = new IslandoraSolrQueryProcessor();

    $solr_build->buildQuery($solr_query, $solr_params);
    $solr_build->executeQuery(FALSE);

    $solr_results = (array) $solr_build->islandoraSolrResult['response']['objects'];
    if ($solr_results) {
      $data = array();
      foreach ($solr_build->islandoraSolrResult["response"]["objects"] as $doc) {
        $data[$doc['PID']] = (array_key_exists('title_sort_ms', $doc) ? $doc['title_sort_ms'][0] : "");
      }
    }
    if (count($t_array) >= $num_pics) {
      $picked = array_rand($t_array, $num_pics);
      $output = '<div id="manidora-homepage-thumbs">';
      foreach ($picked as $pid) {
        $title = '';
        // If $data is an array and has a key of the current PID,
        // then we know the title.
        if (isset($data) && is_array($data) && array_key_exists($t_array[$pid], $data)) {
          $title = 'title="' . check_plain($data[$t_array[$pid]]) . '" ';
        }
        $output .= '<a ' . $title . 'href="' . url("islandora/object/" . $t_array[$pid]) . '" class="manidora-homepage-thumb-link"><img src="' . url("islandora/object/" . $t_array[$pid] . "/datastream/TN/view") . '" alt="Object ' . check_plain($t_array[$pid]) . '" class="manidora-homepage-thumb-img" /></a>' . "\n";
      }
      $output .= "</div> <!-- /#manidora-homepage-thumbs -->\n";
    }
  }
  return $output;
}

/**
 * Generate 4 most recently added items from Solr timestamp.
 */
function __manidora_whats_new_content() {
  $solr_build = new IslandoraSolrQueryProcessor();

  $item_limit = 4;

  $query = 'fgs_label_s:* AND fgs_createdDate_dt:[ * TO * ] AND fedora_datastreams_ms:TN';
  $params = array(
    'fl' => 'PID,fgs_label_s,fgs_createdDate_dt,fedora_datastreams_ms,RELS_EXT_canSeeRestricted_uri_ms,RELS_EXT_restrictedState_literal_ms',
    'sort' => 'fgs_createdDate_dt desc',
    'fq' => array(
      '-RELS_EXT_hasModel_uri_ms:("info:fedora/islandora:newspaperPageCModel" OR "info:fedora/islandora:pageCModel")',
    ),
    'facet' => 'false',
    'hl' => 'false',
    'rows' => $item_limit,
    'limit' => $item_limit,
  );
  $solr_build->buildQuery($query, $params);

  $solr_build->solrParams['fq'] = array_merge($solr_build->solrParams['fq'], $params['fq']);
  $solr_build->solrParams['facet'] = $solr_build->solrParams['hl'] = 'false';
  $solr_build->solrParams = islandora_solr_remove_base_filters($solr_build->solrParams);

  $solr_build->executeQuery(FALSE);
  $solr_results = (array) $solr_build->islandoraSolrResult['response']['objects'];

  $output = "<div class=\"manidora-whats-new-content\">";
  $map = function($o) {
    return array(
      'pid' => $o['PID'],
      'title' => $o['object_label'],
      'date' => $o['solr_doc']['fgs_createdDate_dt'],
    );
  };
  if ($solr_results) {
    $solr_results = array_slice($solr_results, 0, $item_limit);
    $results = array_map($map, $solr_results);
    foreach ($results as $result) {
      $output .= theme('manidora_whats_new_item', array('item' => $result));
    }
  }
  $output .= "</div>";
  return $output;
}

/**
 * Theme the manidora-whats-new list items.
 */
function manidora_preprocess_manidora_whats_new_item(&$variables) {
  global $user;

  $item = $variables['item'];
  $object = islandora_object_load($item['pid']);
  if ($object) {
    $variables['pid'] = $item['pid'];
    $tstamp = DateTime::createFromFormat('Y-m-d\TH:i:s.uZ', $item['date']);
    if ($tstamp) {
      $variables['timestamp'] = $tstamp->format('M j, Y');
    }
    else {
      $variables['timestamp'] = '';
    }
    $title = $item['title'];
    if (module_exists('islandora_restricted')) {
      module_load_include('inc', 'islandora_restricted', 'includes/utilities');
      if (!islandora_restricted_user_has_access($object, $user)) {
        $title .= " - Restricted";
        $image_path = drupal_get_path('module', 'islandora_restricted') . '/images/access_denied.png';
      }
    }
    if (!isset($image_path) && isset($object['TN'])) {
      $image_path = 'islandora/object/' . $item['pid'] . '/datastream/TN/view';
    }
    else if (!isset($image_path)) {
      $image_path = drupal_get_path('module', 'islandora', '/images/folder.png');
    }
    $stitle = (strlen($title) > 30 ? substr($title, 0, strpos(wordwrap($title, 25, '~'), '~')) . '...' : $title);
    $variables['title'] = $title;
    $variables['image'] = t('<a href="@link"><img src="@url" alt="@title" class="featured-thumb"/></a>', array(
      '@url' => url($image_path),
      '@title' => $stitle,
      '@link' => url('islandora/object/' . $item['pid']),
    ));
    $variables['link'] = l($stitle, 'islandora/object/' . $item['pid']);
  }
  unset($variables['item']);
}

/**
 * Check to see if What's New can show this.
 *
 * $doc : Mixed array of a Solr Document
 * $roles : Users roles modified to structured used by islandora_restricted
 *
 * return int : 0 => public, 1 => restricted, 2 => hidden
 */
function __manidora_check_restricted_permissions($doc, $roles) {
  // TODO: Deprecate and remove
  if (array_key_exists('RELS_EXT_restrictedState_literal_ms', $doc)) {
    if (count(array_intersect($roles, $doc['RELS_EXT_canSeeRestricted_uri_ms'])) > 0) {
      // If the person has the role, then it is public to them
      // regardless if it is hidden or restricted.
      return 0;
    }
    elseif (in_array('hidden', $doc['RELS_EXT_restrictedState_literal_ms'])) {
      return 2;
    }
    elseif (in_array('restricted', $doc['RELS_EXT_restrictedState_literal_ms'])) {
      return 1;
    }
  }
  return 0;
}

/**
 * Display the thumnbnail when editing the metadata form.
 */
function __manidora_preview_image() {
  global $user;
  $path = current_path();
  $path_parts = explode('/', $path);
  $view = FALSE;
  $output = "";
  foreach ($path_parts as $p) {
    if (strpos($p, ':') > 0) {
      $pid = $p;
      $obj = islandora_object_load($pid);
      if (!is_null($obj) && FALSE !== $obj) {
        if ($obj->getDatastream('TN') !== FALSE) {
          if (module_exists('islandora_restricted')) {
            module_load_include('inc', 'islandora_restricted', 'includes/utilities');
            if (islandora_restricted_retrieve_restrictions($obj, $user)) {
              $view = TRUE;
              break;
            }
          }
          else {
            $view = TRUE;
          }
        }
      }
      $obj = NULL;
    }
  }
  if ($view) {
    $module_path = drupal_get_path('module', 'manidora');
    drupal_add_library('system', 'ui.draggable');
    drupal_add_js("{$module_path}/js/preview.js");
    $title = t('Preview');
    $url = url('islandora/object/' . $pid . '/datastream/TN/view');
    $output = <<<EOF
<div class="manidora-preview-image" id="image-$pid">
  <h1 class="block-title">$title</h1>
  <img src="$url" />
</div>
EOF;
  }
  return $output;
}

/**
 * Implements hook_form_FORM_ID_alter().
 *
 * Alters the advanced search form to get the role search into it.
 *
 * @param $form
 *   The Drupal form.
 * @param $form_state
 *   The Drupal form state.
 */
function manidora_form_islandora_solr_advanced_search_form_alter(&$form, &$form_state) {
  $solr_role_field = variable_get('manidora_name_role_search_field', NULL);
  $solr_roles_fields = variable_get('manidora_name_role_roles_field', NULL);
  // Make sure we have set the fields before we even try.
  if (!is_null($solr_role_field) && !empty($solr_role_field) &&
    !is_null($solr_roles_fields) && !empty($solr_roles_fields)) {
    $cache = cache_get('manidora_solr_advanced_roleterm_list');
    if ($cache === FALSE) {
      $roleterms = _manidora_advanced_role_get_terms();
      $roleterms = array_merge(['none' => 'Choose a role'], $roleterms);
      cache_set('manidora_solr_advanced_roleterm_list', $roleterms);
    }
    else {
      $roleterms = $cache->data;
    }
    $role_array = [
      '#tree' => TRUE,
      '#prefix' => '<div>',
      '#suffix' => '</div>',
      'field' => [],
      'search' => [],
      'role' => [
        '#type' => 'select',
        '#options' => $roleterms,
        '#states' => [
          'visible' => [
          ],
        ],
      ],
      'hidden_submit' => [],
      'add' => [],
    ];
    if (isset($form['terms'])) {
      foreach ($form['terms'] as $key => &$term) {
        if (is_numeric($key)) {
          if (!in_array('Role/Term', $term['field']['#options'])) {
            $new = array_replace($role_array, $term);
            $new['field']['#options']['name_and_role_term'] = t('Name and Role');
            $new['role']['#states']['visible']["select[name=\"terms[$key][field]\"]"] = ['value' => 'name_and_role_term'];
            $term = $new;
          }
        }
      }
    }
    if (!in_array('manidora_advanced_role_search_validate', $form['#validate'])) {
      $form['#validate'][] = 'manidora_advanced_role_search_validate';
    }
    if (!in_array('manidora_advanced_role_search_submit', $form['#submit'])) {
      $form['#submit'] = array_merge(['manidora_advanced_role_search_submit'], $form['#submit']);
    }
  }
}

/**
 * Implements hook_form_validate().
 *
 * Check that you have selected a role if you are searching Name/Role.
 *
 * @param $form
 *   The Drupal form.
 * @param $form_state
 *   The Drupal form state.
 */
function manidora_advanced_role_search_validate($form, &$form_state) {
  if (isset($form_state['clicked_button']) && strpos($form_state['clicked_button']['#name'], 'op') == 0) {
    if (isset($form_state['values']['terms'])) {
      foreach ($form_state['values']['terms'] as $key => $term) {
        if ($term['field'] == 'name_and_role_term' && $term['role'] == 'none') {
          form_set_error("terms][{$key}][role", t("You must select a role."));
        }
      }
    }
  }
}

/**
 * Implements hook_form_submit().
 *
 * Alter the form if you are searching a Name/Role.
 *
 * @param $form
 *   The Drupal form.
 * @param $form_state
 *   The Drupal form state.
 */
function manidora_advanced_role_search_submit($form, &$form_state) {
  // The Solr field with name and role in it.
  $solr_role_field =  variable_get('manidora_name_role_search_field', NULL);
  if (!is_null($solr_role_field) && !empty($solr_role_field)) {
    if (isset($form_state['clicked_button']) && strpos($form_state['clicked_button']['#name'], 'op') == 0) {
      if (isset($form_state['values']['terms'])) {
        foreach ($form_state['values']['terms'] as $key => &$term) {
          if ($term['field'] == 'name_and_role_term') {
            $term['field'] = $solr_role_field;
            $term['search'] = $term['search'] . " AND " . $term['role'];
          }
        }
      }
    }
  }
}

/**
 * Generate a list of role terms from Solr.
 *
 * @return array
 *   Unique proper case role terms.
 */
function _manidora_advanced_role_get_terms() {
  //$solr_field = 'mods_name_personal_author_role_roleTerm_text_ms';
  $solr_field = variable_get('manidora_name_role_roles_field', NULL);
  $results = array();
  if (!is_null($solr_field) && !empty($solr_field)) {
    $query = '*:*';
    $params = [
      'facet' => 'true',
      'facet.field' => $solr_field,
      "facet.{$solr_field}.mincount" => 1,
      "facet.{$solr_field}.sort" => 'index',
      'rows' => 0,
    ];

    $solr = new IslandoraSolrQueryProcessor();
    $solr->buildQuery($query, $params);
    $solr->solrParams = array_replace_recursive($solr->solrParams, $params);
    $solr->executeQuery();

    $results = [];
    if (isset($solr->islandoraSolrResult['facet_counts']['facet_fields'][$solr_field])) {
      $results = array_keys($solr->islandoraSolrResult['facet_counts']['facet_fields'][$solr_field]);
      array_walk($results, function (&$o) {
        $o = ucfirst(strtolower($o));
      });
      $results = array_unique($results);
      $results = array_combine($results, $results);
    }
  }
  return $results;
}
