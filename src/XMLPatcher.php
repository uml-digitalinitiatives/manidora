<?php
/**
 * @file
 * Patch class object
 */

/**
 * Patch has format of:
 * $patch = array(
 *   'ns' => array(
 *      'prefix1' => 'uri1',
 *      'prefix2' => 'uri2',
 *   ),
 *   'changes' => array(
 *      array('type' => 'replace', 'old' => '<old xpath>', 'new' => '<new XML string>', 'insert_ns' => <bool insert the namespace on the element added>),
 *      array('type' => 'add', 'parent' => '<parent xpath>', 'new' => '<new XML string>', 'insert_ns' => <bool insert the namespace on the element added>),
 *      array('type' => 'remove', 'old' => '<old xpath>'),
 *   ),
 * );
 */

namespace Drupal\manidora;

class XMLPatcher {
    private $patch;

    private $namespaces = array();
    private $dom;
    private $xpath;

    /**
     * Constructor
     *
     * @param array $patch
     *   The patch object.
     */
    public function __construct(array $patch) {
      $this->validate_patch($patch);
      $this->patch = $patch;
    }

    /**
     * Static initializor.
     *
     * @param array $patch
     *   The patch object.
     *
     * @return \Drupal\manidora\XMLPatcher
     *   A new patcher object.
     */
    public static function create(array $patch) {
      return new XMLPatcher($patch);
    }

    /**
     * Static instant patch function
     * @param string $xml
     *   The XML to patch.
     * @param array $patch
     *   The patch object.
     *
     * @return string
     *   The modified XML string.
     */
    public static function patch($xml, array $patch) {
      $p = XMLPatcher::create($patch);
      return $p->apply_patch($xml);
    }

    /**
     * Apply the patch
     *
     * @param string $xml
     *   The XML to patch.
     *
     * @return string
     *   The modified XML string.
     *
     * @throws \Drupal\manidora\XMLPatcherException
     *   Can't load the xml document.
     */
    public function apply_patch($xml) {

      $this->validate_xml($xml);

      $this->dom = new \DOMDocument();
      $this->dom->loadXML($xml);

      if (!$this->dom) {
        throw new XMLPatcherException("Unable to load XML to DOMDocument", 905);
      }

      $this->xpath = new \DOMXpath($this->dom);

      if (isset($this->patch['ns'])) {
        $this->namespaces = $this->patch['ns'];
        foreach ($this->patch['ns'] as $ns => $uri) {
          $this->xpath->registerNamespace($ns, $uri);
        }
      }

      $this->changed = FALSE;
      foreach ($this->patch['changes'] as $change) {
        if ($change['type'] == 'replace') {
          $insert_ns = isset($change['insert_ns']) ? (bool) $change['insert_ns'] : FALSE;
          $this->replace($change['old'], $change['new'], $insert_ns);
        }
        else if ($change['type'] == 'add') {
          $insert_ns = isset($change['insert_ns']) ? (bool) $change['insert_ns'] : FALSE;
          $this->add($change['parent'], $change['new'], $insert_ns);
        }
        else if ($change['type'] == 'remove') {
          $this->remove($change['old']);
        }
      }
      // If nothing changes just return the source doc.
      return $this->changed ? $this->dom->saveXML() : $xml;
    }

    /**
     * Replace one or more nodes from the document, with a new element.
     *
     * @param string $old_xpath
     *   The XPath to the node to replace.
     * @param string $new_xml
     *   The new XML element to insert.
     * @param bool $insert_ns
     *   Whether to insert the namespace prefixes directly on the element.
     *
     * @return void
     */
    private function replace($old_xpath, $new_xml, $insert_ns = FALSE) {
      $hits = $this->xpath->query($old_xpath, null, false);
      if ($hits && count($hits) > 0) {
        if ($insert_ns) {
          $new_xml = XMLPatcher::insert_namespaces($new_xml, $this->namespaces);
        }
        foreach ($hits as $hit) {
          $parent = $hit->parentNode;
          //error_log('new_xml is ' . $new_xml);
          $replace = dom_import_simplexml(simplexml_load_string($new_xml));
          if ($replace !== FALSE) {
            $replace = $this->dom->importNode($replace, TRUE);
            $parent->replaceChild($replace, $hit);
            $this->changed = TRUE;
          }
        }
      }
    }

    /**
     * Remove one or more nodes from the document.
     *
     * @param string $old_xpath
     *   The XPath to the node to remove.
     *
     * @return void
     */
    private function remove($old_xpath) {
      $hits = $this->xpath->query($old_xpath);
      $dom_to_remove = array();
      foreach ($hits as $hit) {
        $dom_to_remove[] = $hit;
      }
      foreach ($dom_to_remove as $hit) {
        $hit->parentNode->removeChild($hit);
        $this->changed = TRUE;
      }
    }

    /**
     * Add a node to the document.
     *
     * @param string $parent_xpath
     *   The XPath to the parent node to append to.
     * @param string $new_xml
     *   The new XML element to insert.
     *
     * @return void
     */
    private function add($parent_xpath, $new_xml, $insert_ns = FALSE) {
      if ($insert_ns) {
        $new_xml = XMLPatcher::insert_namespaces($new_xml, $this->namespaces);
      }
      $hits = $this->xpath->query($parent_xpath);
      foreach ($hits as $hit) {
        $new_node = dom_import_simplexml(simplexml_load_string($new_xml));
        if ($new_node !== FALSE) {
          $new_node = $this->dom->importNode($new_node, TRUE);
          $hit->appendChild($new_node);
          $this->changed = TRUE;
        }
      }
    }

    /**
     * Insert the namespaces to the element to allow us to parse an entire element.
     *
     * @param string $element
     *   The XML element string.
     * @param array $namespaces
     *   The array of prefixes and namespaces.
     *
     * @return string
     *   The modified XML element with the namespaces added.
     */
    protected static function insert_namespaces($element, array $namespaces) {
      $pos = strpos($element, '>');
      if (strpos(substr($element, 0, $pos), '/') !== FALSE) {
        // We may have an empty element, parse to see if the
        // backslash is inside an attribute.
        for ($x = $pos; $x > 0; $x -= 1) {
          if (substr($element, $x, 1) == "/") {
            $pos = $x;
            break;
          }
          elseif (substr($element, $x, 1) == "\"" || substr($element, $x, 1) == "'") {
              break;
          }
        }
      }
      $add_string = "";
      foreach ($namespaces as $prefix => $uri) {
        if (strpos($element, "xmlns:$prefix") === FALSE) {
          $add_string .= " xmlns:$prefix=\"$uri\"";
        }
      }
      return substr($element, 0, $pos) . $add_string . substr($element, $pos);
    }

    /**
     * Validate that the patch has been constructed correctly and populates the $this->operations array.
     *
     * @param array $patch
     *   The patch.
     *
     * @return void
     *   Throws an Exception if patch is incorrect.
     *
     * @throws \Drupal\manidora\XMLPatcherException
     *   On invalid patch structure or missing elements.
     */
    private function validate_patch(array $patch) {
      $namespaces = array();
      if (isset($patch['ns'])) {
        if (!is_array($patch['ns'])) {
          throw new XMLPatcherException('Namespaces section of patch must contain an array', 904);
        }
        else {
          foreach ($patch['ns'] as $prefix => $uri) {
            if (is_int($prefix)) {
              throw new XMLPatcherException('Namespaces in patch must be an associative array of prefix => URI.', 904);
            }
            if (isset($namespaces[$prefix]) && $namespaces[$prefix] != $uri) {
              throw new XMLPatcherException(sprintf('More than one namespace has the prefix (%s) but has different URIs.', $prefix), 904);
            }
            $namespaces[$prefix] = $uri;
          }
        }
      }
      if (isset($patch['changes'])) {
        if (is_array($patch['changes'])) {
          foreach ($patch['changes'] as $change) {
            if (!isset($change['type'])){
              throw new XMLPatcherException('Patch has a change without a "type": ' . implode(' - ', $change));
            }
            if ($change['type'] == 'replace' && !(isset($change['old']) && isset($change['new']))) {
              throw new XMLPatcherException('Replace patch is missing a required element (old|new)', 903);
            }
            else if ($change['type'] == 'add' && !(isset($change['parent']) && isset($change['new']))) {
              throw new XMLPatcherException('Add patch is missing a required element (parent|new)', 903);
            }
            else if ($change['type'] == 'remove' && !(isset($change['old']))) {
              throw new XMLPatcherException('Remove patch is missing a required element (old)', 903);
            }
          }
        }
      }
    }

    /**
     * Validate the input XML.
     *
     * @param string $xml
     *   The input XML.
     *
     * @throws \Drupal\manidora\XMLPatcherException
     *   Throws a ParserException on error.
     */
    private function validate_xml($xml) {
      // Validate the inserted XML
      libxml_use_internal_errors(true);

      $doc = simplexml_load_string($xml);
      $xml_lines = explode("\n", $xml);

      if ($doc === FALSE) {
        $errors = libxml_get_errors();

        $exception = "";
        foreach ($errors as $error) {
            $exception .= XMLPatcher::display_xml_error($error, $xml_lines);
        }
        libxml_clear_errors();
        throw new XMLPatcherException("Invalid XML provided: \n$exception", 905);
      }
    }

    /**
     * Format XML errors and match to lines in XML.
     *
     * @param Object $error
     *   libxml error object.
     * @param array $xml
     *   original xml split on newline characters.
     *
     * @return string
     *   Formatted error message.
     */
    private function display_xml_error($error, $xml) {
      $return  = $xml[$error->line - 1] . "\n";
      $return .= str_repeat('-', $error->column) . "^\n";

      switch ($error->level) {
        case \LIBXML_ERR_WARNING:
          $return .= "Warning $error->code: ";
          break;

         case \LIBXML_ERR_ERROR:
          $return .= "Error $error->code: ";
          break;

        case \LIBXML_ERR_FATAL:
          $return .= "Fatal Error $error->code: ";
          break;

      }

      $return .= trim($error->message) .
         "\n  Line: $error->line" .
         "\n  Column: $error->column";

      if ($error->file) {
        $return .= "\n  File: $error->file";
      }

      return "$return\n\n--------------------------------------------\n\n";
    }
}

class XMLPatcherException extends \Exception {
  /**
   * Codes
   *
   * 903 - Patch changes syntax error
   * 904 - Patch namespace syntax error
   * 905 - Bad XML
   */

}
