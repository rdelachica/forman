<?php
// $Id: homebox.admin.inc,v 1.2.2.33 2010/08/11 00:12:34 mikestefff Exp $

/**
 * @file
 * Homebox admin file, takes care admin interface for homebox
 *
 * Defines homebox pages, default layout, settings
 */

function homebox_admin_page() {
  // Already validated
  $page = homebox_get_page(arg(4));
  
  if ($page) {
    drupal_set_title(t('Edit !title', array('!title' => $page->settings['title'])));
  }
  
  if (!$page) {
    $form['page'] = array(
      '#type' => 'fieldset',
      '#title' => t('Add a new page'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
  }
  
  $form['page']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Machine name'),
    '#required' => TRUE,
    '#size' => 32,
    '#maxlength' => 64,
    '#description' => t('The machine readable name of this page. It must be unique, and it must contain only alphanumeric characters and underscores. Once created, you will not be able to change this value!'),
  );
  
  if ($page) {
    $form['page']['name']['#value'] = $page->name;
    $form['page']['name']['#disabled'] = TRUE;
  }
  
  $form['page']['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Homebox options'),
    '#collapsible' => TRUE,
  );
  
  if (!$page) {
    $form['page']['options']['#collapsed'] = TRUE;
  }

  $form['page']['options']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#size' => 32,
    '#default_value' => $page->settings['title'],
    '#maxlength' => 64,
    '#prefix' => theme('advanced_help_topic', 'homebox', 'new-page'),
    '#description' => t('The title of the page that will be created.'),
  );
  
  $form['page']['options']['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#size' => 32,
    '#default_value' => $page->settings['path'],
    '#maxlength' => 255,
    '#description' => t('Optionally specify an alternative URL by which this page can be accessed. For example, type "dashboard" when creating a Dashboard page. Use a relative path and don\'t add a trailing slash or the URL alias won\'t work.'),
  );
  
  $form['page']['options']['menu'] = array(
    '#type' => 'checkbox',
    '#title' => t('Create a menu entry'),
    '#description' => t('If checked, a menu entry will be created for this page.'),
    '#default_value' => $page ? $page->settings['menu'] : 1,
  );
  
  $form['page']['options']['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable the page'),
    '#description' => t('If unchecked, only users with the <i>administer homebox</i> permission will be able to view this page.'),
    '#default_value' => $page ? $page->settings['enabled'] : 1,
  );
  
  $form['page']['options']['full'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable block regions'),
    '#description' => t('If checked, the theme block regions will be disabled for this page, giving more room for the homebox.'),
    '#default_value' => $page ? $page->settings['full'] : 0,
  );
  
  $form['page']['options']['custom'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow custom items'),
    '#description' => t('If checked, logged in users will be able to add custom blocks to their own page. The blocks contain a user-entered title and content body.'),
    '#default_value' => $page ? $page->settings['custom'] : 0,
  );
  
  $result = db_query('SELECT rid, name FROM {role} ORDER BY name');
  $role_options = array();
  while ($role = db_fetch_object($result)) {
    $role_options[$role->name] = $role->name;
  }
  $form['page']['options']['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Allow only certain roles to access the page'),
    '#default_value' => count($page->settings['roles']) ? $page->settings['roles'] : array('authenticated user'),
    '#options' => $role_options,
    '#description' => t('Select which roles can view the page.'),
  );
  
  if ($page) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save page'),
      '#weight' => 2,
    );
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => homebox_page_is_api($page->name) ? t('Revert page') : t('Delete page'),
      '#weight' => 3,
    );
  }
  else {
    $form['page']['import_fieldset'] = array(
      '#type' => 'fieldset',
      '#title' => 'Import Homebox',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['page']['import_fieldset']['import'] = array(
      '#title' => t('Import data'),
      '#type' => 'textarea',
      '#description' => t('Optionally include an exported Homebox to import settings and block layout. Ensure that the path is currently not in use. The machine name supplied in the import data will be ignored.'),
    );
    $form['page']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Add page'),
    );
    
    $form['#submit'][] = 'homebox_admin_page_submit';
    $form['#validate'][] = 'homebox_admin_page_validate';
  }
  
  return $form;
}

/**
 * Theme the new-page form.
 *
 * @ingroup themeable
 */
function theme_homebox_admin_new_page($form) {
  // Check PHP version
  if (!_homebox_check_php()) {
    drupal_set_message(t('Homebox 2 requires PHP version 5.2 or higher. You are currently running PHP !version.', array('!version' => phpversion())), 'error');
  }
  
  // Make sure that Homebox 2 was installed clean since
  // there is no upgrade path between 1.x and 2.x
  if (variable_get('homebox_version', 0) != HOMEBOX_VERSION) {
    drupal_set_message(t('Homebox 2 was not properly installed. You must completely uninstall and install Homebox 2 now.'), 'error'); 
  }
  
  $pages = homebox_pages();
  $output = drupal_render($form);

  $header = array(t('Name'), array('data' => t('Operations'), 'colspan' => 5));
  if (is_array($pages) && !empty($pages)) {
    foreach ($pages as $page) {
      $link = l($page->settings['title'], $page->settings['path']);
      $edit = l(t('Edit'), 'admin/build/homebox/edit/' . $page->name);
      $layout = l(t('Layout'), 'admin/build/homebox/layout/' . $page->name);
      $settings = l(t('Settings'), 'admin/build/homebox/settings/' . $page->name);
      $flush = l(t('Flush'), 'admin/build/homebox/flush/' . $page->name);
      $export = l(t('Export'), 'admin/build/homebox/export/' . $page->name);
      $rows[] = array($link, $edit, $layout, $settings, $flush, $export);
    }
  }
  
  $output .= theme('table', $header, $rows);

  return $output;
}

function homebox_admin_page_validate($form, &$form_state) {
  // No valifation needed on Delete or Revert
  if ($form_state['values']['op'] == t('Delete page') ||
    $form_state['values']['op'] == t('Revert page')) {
    return; 
  }

  // If no imported settings...
  if (!$form_state['values']['import']) {
    // Filter title
    $form_state['values']['title'] = filter_xss($form_state['values']['title']);
    
    // Make sure a title is present
    if (!$form_state['values']['title']) {
      form_set_error('title', t('You must enter a page title.')); 
    }
    
    // Check if the path exists
    if (!$form_state['values']['path']) {
      form_set_error('path', t('You must enter a path.')); 
    }
    // If so, make sure it's valid
    else {
      homebox_check_path($form_state['values']['path'], $form_state['values']['name'], 'path');
    }
    
    // Check machine name
    if ($form_state['values']['name'] && $form_state['values']['op'] == t('Add page')) {
      homebox_check_name($form_state['values']['name'], 'name');
    }
  }
  else {
    /*
     * The following checks the import data object and validates
     * all of the components as done above
     */
    // Add object back to the form
    $form_state['values']['import'] = homebox_check_page_object(
      $form_state['values']['import'], 
      $form_state['values']['name'],
      'import'
    );
  }
}

function homebox_admin_page_submit($form, &$form_state) {
  $op = $form_state['values']['op'];
  
  if ($op == t('Delete page') || $op == t('Revert page')) {
    // Redirect to the confirmation message
    // Reverting and deleting is the same operation
    $form_state['redirect'] = 'admin/build/homebox/delete/' . $form_state['values']['name'];
    return;
  }
  else if ($op == t('Add page')) {
    // Build page object
    $page = new stdClass;
    $page->name = $form_state['values']['name'];
    
    // Check to see if imported homebox object is present
    if ($form_state['values']['import']) {
      $page->settings = $form_state['values']['import']->settings;
    }
    else {
      // Build default settings
      $page->settings = array(
        'regions' => HOMEBOX_DEFAULT_REGIONS,
        'cache' => 0,
        'color' => 0,
        'colors' => array(),
        'blocks' => array(),
        'widths' => array(),
      );
      
      // Map form settings to page object
      homebox_admin_form_to_page($page, $form_state);
    }
    
    // Save page
    homebox_save_page($page);
    
    drupal_set_message(t('The page has been added.'));
  }
  else if ($op == t('Save page')) {
    // Fetch old page to amend
    $page = homebox_get_page($form_state['values']['name']);
    
    // Map form settings to page object
    homebox_admin_form_to_page($page, $form_state);
    
    // Save settings
    homebox_save_page($page);
    
    drupal_set_message(t('Changes have been saved'));
  }
  
  $form_state['redirect'] = 'admin/build/homebox';
  menu_rebuild();
  return;
}

/**
 * Confirmation form to delete/revert a homebox page.
 *
 * @return
 *   A confirmation form to delete homebox page
 */
function homebox_admin_page_delete_confirm(&$form_state, $page) {
  $form['#operation'] = homebox_page_is_api($page->name) ? t('revert') : t('delete');
  $form['#page'] = $page;
  
  $caption .= '<p>'. t('This action cannot be undone.') .'</p>';
  return confirm_form($form, t('Are you sure you want to !operation homebox page %title?', array('!operation' => $form['#operation'], '%title' => $page->settings['title'])), 'admin/build/homebox', $caption, ucfirst($form['#operation']));
}

/**
 * Submission of the delete/revert confirmation form
 */
function homebox_admin_page_delete_confirm_submit($form, &$form_state) {
  // Delete/revert the page
  homebox_delete_page($form['#page']->name);
  
  if ($form['#operation'] == t('revert')) {
    drupal_set_message(t('The page has been reverted.'));
  }
  else {
    drupal_set_message(t('The page has been deleted.'));
  }
  
  menu_rebuild();

  $form_state['redirect'] = 'admin/build/homebox';
}

/*
 * Map settings from the form homebox_admin_page to
 * a page object
 * 
 * @param $page
 *   A page object
 * @param $form_state
 *   The form state on form submission
 */
function homebox_admin_form_to_page(&$page, $form_state) {
  $page->settings['title'] = $form_state['values']['title'];
  $page->settings['path'] = $form_state['values']['path'];
  $page->settings['menu'] = (int) $form_state['values']['menu'];
  $page->settings['enabled'] = (int) $form_state['values']['enabled'];
  $page->settings['full'] = (int) $form_state['values']['full'];
  $page->settings['custom'] = (int) $form_state['values']['custom'];
  $page->settings['roles'] = array();
    
  foreach ($form_state['values']['roles'] as $role) {
    if ($role) {
      $page->settings['roles'][] = $role;
    }
  }
}

/**
 * Form builder function for module settings.
 */
function homebox_layout($page) {
  drupal_set_title(t('!page_name layout', array('!page_name' => $page->settings['title'])));

  // Gets admin build block helper for usort function
  require_once(drupal_get_path('module', 'block') .'/block.admin.inc');

  // Fetch and sort blocks
  $blocks = _block_rehash();

  $home_blocks = $page->settings['blocks'];

  foreach ($blocks as $key => &$block) {
    // We don't want to list exposed views blocks
    if (strpos($block['delta'], '-exp-') === 0) {
       // Remove exposed views blocks
       unset($blocks[$key]);
       continue;
    }
    
    // Generate identified for current block
    $id = $block['module'] . '_' . $block['delta'];

    // Check if current block has settings
    if (isset($home_blocks[$id])) {
      $hb = $home_blocks[$id];
      $block['weight']  = $hb['weight'];
      $block['movable'] = (bool) $hb['movable'];
      $block['status']  = (bool) $hb['status'];
      $block['open']    = (bool) $hb['open'];
      $block['closable'] = (bool) $hb['closable'];
      $block['title'] = $hb['title'];
    }
    // Set defaults.
    else {
      $block['weight']  = 99;
      $block['movable'] = TRUE;
      $block['status']  = TRUE;
      $block['open']    = TRUE;
      $block['closable'] = TRUE;
    }
  }

  usort($blocks, '_homebox_block_compare');

  return drupal_get_form('homebox_admin_display_form', $blocks, $page, $theme = NULL);
}

/**
 * Generate a page export form
 */
function homebox_export_form(&$form_state, $page) {
  $code = '$homebox = new stdClass;' . "\n";
  $code .= '$homebox->name = \'' . $page->name . '\';' . "\n";
  $code .= '$homebox->settings = ' . var_export($page->settings, TRUE) . ';';
  
  $form['export'] = array(
    '#title' => $page->name,
    '#type' => 'textarea',
    '#rows' => 30,
    '#value' => $code,
  ); 
  
  return $form;
}

/**
 * Generate main blocks administration form.
 */
function homebox_admin_display_form(&$form_state, $blocks, $page, $theme = NULL) {
  global $theme_key, $custom_theme;

  // Add CSS
  drupal_add_css(drupal_get_path('module', 'block') .'/block.css', 'module', 'all', FALSE);

  // If non-default theme configuration has been selected, set the custom theme.
  $custom_theme = isset($theme) ? $theme : variable_get('theme_default', 'garland');

  $columns = homebox_named_columns($page->settings['regions']);

  $block_regions = $columns + array(HOMEBOX_REGION_NONE => '<'. t('none') .'>');

  // Weights range from -delta to +delta, so delta should be at least half
  // of the amount of blocks present. This makes sure all blocks in the same
  // region get an unique weight.
  $weight_delta = round(count($blocks) / 2);

  // Build form tree
  $form = array(
    '#tree' => TRUE,
  );
  // Iterate on each block
  foreach ($blocks as $i => $block) {
    $key = $block['module'] .'_'. $block['delta'];
    $form[$key]['module'] = array(
      '#type' => 'value',
      '#value' => $block['module'],
    );
    $form[$key]['delta'] = array(
      '#type' => 'value',
      '#value' => $block['delta'],
    );
    $form[$key]['info'] = array(
      '#value' => check_plain($block['info'])
    );

    $form[$key]['weight'] = array(
      '#type' => 'weight',
      '#default_value' => $block['weight'],
      '#delta' => $weight_delta,
    );

    // Get default region/column for block
    $column = $page->settings['blocks'][$key]['region'];
    // If not set assign to <none>
    $column = ($column != 0) ? $column : HOMEBOX_REGION_NONE;

    $form[$key]['region'] = array(
      '#type' => 'select',
      '#default_value' => $column,
      '#options' => $block_regions,
    );
    $form[$key]['title'] = array(
      '#type' => 'textfield',
      '#default_value' => $block['title'],
      '#size' => 15,
    );
    $form[$key]['status'] = array(
      '#type' => 'checkbox',
      '#default_value' => $block['status'],
    );
    $form[$key]['open'] = array(
      '#type' => 'checkbox',
      '#default_value' => $block['open'],
    );
    $form[$key]['movable'] = array(
      '#type' => 'checkbox',
      '#default_value' => $block['movable'],
    );
    $form[$key]['closable'] = array(
      '#type' => 'checkbox',
      '#default_value' => $block['closable'],
    );
    $form[$key]['bid'] = array(
      '#type' => 'value',
      '#value' => $block['bid'],
    );
  }

  $form['name'] = array(
    '#type' => 'value',
    '#value' => $page->name,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save blocks'),
  );

  return $form;
}

/**
 * Process main home blocks administration form submission.
 */
function homebox_admin_display_form_submit($form, &$form_state) {
  // Page
  $page = homebox_get_page($form_state['values']['name']);

  // We can safely remove old records
  $page->settings['blocks'] = array();

  foreach ($form_state['values'] as $key => $block) {
    // Check to see if this is a block
    // We check $block['region'] == 0, since we don't want to store block that are not enabled
    if (is_array($block) && is_numeric($block['bid']) && $block['bid'] != 0 && is_numeric($block['region']) && (int) $block['region'] > 0) {
      $page->settings['blocks'][$key] = array(
        'module' => $block['module'],
        'delta' => $block['delta'],
        'region' => (int) $block['region'],
        'movable' => (int) $block['movable'],
        'status' => (int) $block['status'],
        'open' => (int) $block['open'],
        'closable' => (int) $block['closable'],
        'title' => $block['title'],
        'weight' => (int) $block['weight'],
      );
    }
  }

  // Save the page
  homebox_save_page($page);
  
  /*
   * There is no reason to remove the blocks from user settings that
   * were removed here. Blocks that aren't in the page will be stripped
   * from the user settings upon rendering. Once the user saves, the
   * settings will be updated.
   */
   
  menu_rebuild();
  drupal_set_message(t('Layout settings have been updated for !page_title.', array('!page_title' => $page->settings['title'])));
  $form_state['redirect'] = 'admin/build/homebox';
}

/*
 * Provide form to flush user settings for a given page
 */
function homebox_admin_flush_form(&$form_state, $page) {
  $form['name'] = array(
    '#type' => 'value',
    '#value' => $page->name,
  );

  $form = confirm_form(
    $form, 
    t('Are you sure you want to flush user settings for !name?', array('!name' => $page->name)), 
    'admin/build/homebox',
    t('Executing this action will delete all user\'s settings for this page'),
    t('Flush settings')
  ); 
  
  return $form;
}

/*
 * Submit homebox_admin_flush_form()
 */
function homebox_admin_flush_form_submit(&$form, &$form_state) {
  // Load the page
  $page = homebox_get_page($form_state['values']['name']);
  if ($page) {
    if (homebox_flush_settings($page)) {
      drupal_set_message(t('Settings flushed successfully for !name', array('!name' => $page->name))); 
    }
    else {
      drupal_set_message(t('An error has occurred while trying to flush settings for !name', array('!name' => $page->name)));
    }
  }
  else {
    drupal_set_message(t('An error occured has occured'), 'error');
  }
  
  $form_state['redirect'] = 'admin/build/homebox';
}

/**
 * Process variables for homebox-admin-display.tpl.php.
 *
 * @param $variables
 * @see theme_homebox_admin_display()
 */
function template_preprocess_homebox_admin_display_form(&$variables) {
  global $theme_key;
  drupal_add_css($path = drupal_get_path('module', 'homebox') .'/homebox.css', $type = 'module', $media = 'all', $preprocess = TRUE);

  // Load page
  $page = homebox_get_page($variables['form']['name']['#value']);
  $block_regions = homebox_named_columns($page->settings['regions']);
  $variables['block_regions'] = $block_regions + array(HOMEBOX_REGION_NONE => t('Disabled'));
  foreach ($block_regions as $key => $value) {
    // Initialize an empty array for the region.
    $variables['block_listing'][$key] = array();
  }

  // Initialize disabled blocks array.
  $variables['block_listing'][BLOCK_REGION_NONE] = array();

  // Set up to track previous region in loop.
  $last_region = '';
  foreach (element_children($variables['form']) as $i) {
    $block = &$variables['form'][$i];
    // Only take form elements that are blocks.
    if (isset($block['info'])) {
      // Fetch region for current block.
      $region = $block['region']['#default_value'];

      // Set special classes needed for table drag and drop.
      $variables['form'][$i]['region']['#attributes']['class'] = 'block-region-select block-region-'. $region;
      $variables['form'][$i]['weight']['#attributes']['class'] = 'block-weight block-weight-'. $region;

      $variables['block_listing'][$region][$i]->row_class = isset($block['#attributes']['class']) ? $block['#attributes']['class'] : '';
      $variables['block_listing'][$region][$i]->block_modified = isset($block['#attributes']['class']) && strpos($block['#attributes']['class'], 'block-modified') !== FALSE ? TRUE : FALSE;
      $variables['block_listing'][$region][$i]->block_title =  drupal_render($block['info']);
      $variables['block_listing'][$region][$i]->region_select = drupal_render($block['region']) . drupal_render($block['theme']);
      $variables['block_listing'][$region][$i]->title = drupal_render($block['title']);
      $variables['block_listing'][$region][$i]->weight_select = drupal_render($block['weight']);
      $variables['block_listing'][$region][$i]->status = drupal_render($block['status']);
      $variables['block_listing'][$region][$i]->open = drupal_render($block['open']);
      $variables['block_listing'][$region][$i]->movable = drupal_render($block['movable']);
      $variables['block_listing'][$region][$i]->closable = drupal_render($block['closable']);
      $variables['block_listing'][$region][$i]->bid = drupal_render($block['bid']);
      $variables['block_listing'][$region][$i]->printed = FALSE;

      $last_region = $region;
    }
  }

  $variables['form_submit'] = drupal_render($variables['form']);
}

/**
 * Forms for administration settings
 *
 * @param $form_state
 * @return
 *  $form
 */
function homebox_configure_form(&$form_state, $page) {
  drupal_set_title(t('!page_name settings', array('!page_name' => $page->settings['title'])));

  $form['columns'] = array(
    '#type' => 'select',
    '#title' => t('Number of columns'),
    '#default_value' => $page->settings['regions'],
    '#options' => array('1' => 1, '2' => 2, '3' => 3, '4' => 4, '5' => 5, '6' => 6, '7' => 7, '8' => 8, '9' => 9),
    '#description' => t('Set the number of columns you want to activate for this Homebox page.'),
  );

  $form['cache'] = array(
    '#type' => 'select',
    '#title' => t('Use blocks cache?'),
    '#default_value' => $page->settings['cache'],
    '#options' => array(0 => t('Don\'t use'), 1 => t('Use if available')),
    '#description' => t('Homebox will use the blocks cache if available for the rendering of the blocks. This can greatly improve performances, but has the drawback to break Views blocks that uses Ajax AND are using <strong>Block settings: Caching</strong>. You can use this option only if the Views blocks used in your homebox that use Ajax are set to <strong>Block settings: Caching:</strong> Do not cache.'),
  );
  
  $form['widths'] = array(
    '#type' => 'fieldset',
    '#title' => t('Custom column widths'),
    '#description' => t('
      Optionally set custom widths for each available region. 
      If these values are omitted, the regions will all be the same width, set horizontally.<br/>
      <img src="' . base_path() . drupal_get_path('module', 'homebox') . '/images/layout-example.png" alt="Layout" style="float:right;"/>
      To achieve a layout like the image to the right, you would set the column count to 4, with widths of 100, 50, 50, and 100.
    '),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  
  for ($i=1; $i <= 9; $i++) {
    $form['widths']['width_' . $i] = array(
      '#type' => 'textfield',
      '#title' => t('Column #!number width', array('!number' => $i)),
      '#default_value' => $page->settings['widths'][$i] ? $page->settings['widths'][$i] : '',
      '#size' => 10,
      '#field_suffix' => '%',
    );
  }

  $form['color'] = array(
    '#type' => 'fieldset',
    '#title' => t('Boxes colors customization'),
    '#description' => t('Here you can set colors that users can use to customize their boxes.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['color']['colors_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Can users set custom colors for each boxes?'),
    '#default_value' => $page->settings['color'],
  );

  if (module_exists('colorpicker')) {
    $form['color']['homebox_colorpicker'] = array(
      '#type' => 'colorpicker',
      '#title' => t('Color picker'),
      '#description' => t('Click an input to choose a color.'),
    );

    for ($i=0; $i < HOMEBOX_NUMBER_OF_COLOURS; $i++) {
      $form['color']['block_color_' . $i] = array(
        '#type' => 'colorpicker_textfield',
        '#title' => t('Color #!number', array('!number' => $i)),
        '#description' => t('Enter an hexadecimal value prefixed with #.'),
        '#default_value' => $page->settings['colors'][$i] ? $page->settings['colors'][$i] : '#E4F0F8',
        '#colorpicker' => 'homebox_colorpicker',
        '#size' => 7,
        '#maxlength' => 7,
      );
    }
  }
  else {
    for ($i=0; $i < HOMEBOX_NUMBER_OF_COLOURS; $i++) {
      $form['color']['block_color_' . $i] = array(
        '#type' => 'textfield',
        '#title' => t('Color #!number', array('!number' => $i)),
        '#description' => t('Enter an hexadecimal value, ex: #ff33dd'),
        '#default_value' => $page->settings['colors'][$i] ? $page->settings['colors'][$i] : '#E4F0F8',
        '#size' => 7,
        '#maxlength' => 7,
      );
    }
    $form['color']['message'] = array(
      '#type' => 'item',
      '#value' => t('If you had the <a href="@url">Color picker</a> module enabled you could choose colors more easily.', array('@url' => 'http://drupal.org/project/colorpicker')),
    );
  }

  $form['name'] = array(
    '#type' => 'value',
    '#value' => $page->name,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Forms for administration settings
 *
 * @param $form
 * @param $form_state
 */
function homebox_configure_form_validate($form, &$form_state) {
  // Make sure the column choice is between 1-9
  $columns = (int) $form_state['values']['columns'];
  if ($columns < 1 ||  $columns > 9) {
    form_set_error('columns', t('You must enter a value between 1 and 9.'));
  }
  
  // Check colors for string length and format - not valid HTML colors!
  for ($i=0; $i < HOMEBOX_NUMBER_OF_COLOURS; $i++) {
    if (strlen($form_state['values']['block_color_' . $i]) != 7) {
      form_set_error('block_color_' . $i, t('Colors must begin with a # and follow by 6 characters.'));
    }
    
    if (substr_count($form_state['values']['block_color_' . $i], '#') != 1) {
      form_set_error('block_color_' . $i, t('Colors must begin with a # and follow by 6 characters.'));
    }
  }
  
  // Validate custom column widths, if any
  $widths = 0;
  for ($i=1; $i <= 9; $i++) {
    if ($form_state['values']['width_' . $i]) {
      $widths++; 
    }
  }
  
  // Only act if widths were entered
  if ($widths) {
    if ($widths != $columns) {
      form_set_error('widths', t('When setting custom widths, you must specify exactly the amount of columns that are available.'));  
    }
    else {
      $widths = 0;
      // Make sure values are numeric, between 1 and 100
      for ($i=1; $i <= $columns; $i++) {
        if (!is_numeric($form_state['values']['width_' . $i]) ||
          $form_state['values']['width_' . $i] > 100 ||
          $form_state['values']['width_' . $i] < 0) {
          form_set_error('width_' . $i, t('Custom column width values must be numeric and in between 0 and 100.'));
        }
        $widths++;
      }

      // Check the amount of widths again to make sure that all
      // values were consecutive
      if ($widths != $columns) {
        form_set_error('widths', t('Custom widths must be entered consecutively.')); 
      }
    }
  }
}

/**
 * Forms for administration settings
 *
 * @param $form
 * @param $form_state
 */
function homebox_configure_form_submit($form, &$form_state) {
  // Fetch page
  $page = homebox_get_page($form_state['values']['name']);

  // Adjust settings
  $page->settings['regions'] = (int) $form_state['values']['columns'];
  $page->settings['cache'] = (int) $form_state['values']['cache'];
  $page->settings['color'] = (int) $form_state['values']['colors_enabled'];
  
  // Adjust colors
  for ($i=0; $i < HOMEBOX_NUMBER_OF_COLOURS; $i++) {
    $page->settings['colors'][$i] = $form_state['values']['block_color_' . $i];
  }
  
  // Adjust widths
  $page->settings['widths'] = array();
  if ($form_state['values']['width_1']) {
    for ($i=1; $i <= $page->settings['regions']; $i++) {
      $page->settings['widths'][$i] = (int) $form_state['values']['width_' . $i]; 
    }
  }
  
  // If blocks are available and set on regions that were removed, move them!
  foreach ($page->settings['blocks'] as $key => $block) {
    // Check block region
    if ($block['region'] > $page->settings['regions']) {
      $page->settings['blocks'][$key]['region'] = 1;
    }
  }

  // Save settings
  homebox_save_page($page);
  
  // Clear out the menu
  menu_rebuild();
  
  drupal_set_message(t('The setting have been saved.'));
  $form_state['redirect'] = 'admin/build/homebox';
}

/**
 * Helper function for sorting blocks on admin/build/homebox.
 *
 * Active blocks are sorted by region, then by weight.
 * Disabled blocks are sorted by name.
 *
 * @param $a
 *  An array
 * @param $b
 *  An array
 * @return
 *  Comparison result
 */
function _homebox_block_compare($a, $b) {
  // Sort by weight.
  $weight = $a['weight'] - $b['weight'];
  if ($weight) {
    return $weight;
  }
  // Sort by title.
  return strcmp($a['info'], $b['info']);
}

/**
 * Returns a keyed array containing regions/columns named base on cols
 *
 * @return
 *  A keyed array like array(1 => 'Column 1', 2 => 'Column 2')
 */
function homebox_named_columns($cols) {
  $columns = array();
  for ($i = 1; $i <= $cols; $i++) {
    $columns[$i] = t('Column !count', array('!count' => $i));
  }
  return $columns;
}

/*
 * Provide form for user profile settings
 * 
 * This allows an admin to specify a Homebox to reside
 * as a profile tab
 */
function homebox_user_settings_page() {
  $form = array();
  
  // Gather all available pages
  $pages = homebox_pages();
  
  // Build form options
  $options = array();
  if ($pages) {
    foreach ($pages as $page) {
      $options[$page->name] = $page->settings['title'] . ' (' . $page->name . ')'; 
    }
  }

  $form['help'] = array(
    '#type' => 'item',
    '#value' => t('
      The permissions set in the chosen Homeboxes will be still be checked. 
      Make sure that users who can view their own profiles can also view the selected Homebox. 
      The tab will only be presented if the current user is viewing their own profile.'
    ),
  );
  
  $form['homebox_user_tab'] = array(
    '#type' => 'select',
    '#title' => t('Add a profile tab'),
    '#default_value' => variable_get('homebox_user_tab', 0),
    '#options' => array(0 => '- None -') + $options,
    '#description' => t('Optionally add a Homebox as a tab to a user profile.'),
  );
  
  $form['#submit'][] = 'menu_rebuild';
  
  return system_settings_form($form);
}
