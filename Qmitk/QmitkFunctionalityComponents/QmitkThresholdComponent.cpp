/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$ 

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "QmitkThresholdComponent.h"
#include "QmitkThresholdComponentGUI.h"

#include <QmitkDataTreeComboBox.h>

#include "mitkRenderWindow.h"
#include "mitkRenderingManager.h"
#include "mitkProperties.h"

#include <qlineedit.h>
#include <qslider.h>
#include <qgroupbox.h>


/***************       CONSTRUCTOR      ***************/
QmitkThresholdComponent::QmitkThresholdComponent(QObject *parent, const char *name, QmitkStdMultiWidget *mitkStdMultiWidget, mitk::DataTreeIteratorBase* it)
{
  SetDataTreeIterator(it);
  m_GUI = NULL;
  m_Node = it->Get();
  m_MultiWidget= mitkStdMultiWidget;
  m_Active = false;
}

/***************       CONSTRUCTOR      ***************/
QmitkThresholdComponent::QmitkThresholdComponent(QObject *parent, const char *name, mitk::DataTreeIteratorBase* it, bool updateSelector, bool showSelector)
: m_UpdateSelector(updateSelector), m_ShowSelector(showSelector)
{
  SetDataTreeIterator(it);
}

/***************        DESTRUCTOR      ***************/
QmitkThresholdComponent::~QmitkThresholdComponent()
{

}

/*************** GET FUNCTIONALITY NAME ***************/
QString QmitkThresholdComponent::GetFunctionalityName()
{
  return m_Name;
}

/*************** SET FUNCTIONALITY NAME ***************/
void QmitkThresholdComponent::SetFunctionalityName(QString name)
{
  m_Name = name;
}

/*************** GET FUNCCOMPONENT NAME ***************/
QString QmitkThresholdComponent::GetFunctionalityComponentName()
{
  return m_Name;
}

/*************** GET DATA TREE ITERATOR ***************/
mitk::DataTreeIteratorBase* QmitkThresholdComponent::GetDataTreeIterator()
{
  return m_DataTreeIterator.GetPointer();
}

/*************** SET DATA TREE ITERATOR ***************/
void QmitkThresholdComponent::SetDataTreeIterator(mitk::DataTreeIteratorBase* it)
{
  m_DataTreeIterator = it;
  m_Node = m_DataTreeIterator->Get();
}

/***************         GET GUI        ***************/
QWidget* QmitkThresholdComponent::GetGUI()
{
  return m_GUI;
}

/*************** TREE CHANGED (       ) ***************/
void QmitkThresholdComponent::TreeChanged()
{

  
  m_ThresholdImageNode->SetData(m_Node->GetData());
  ShowThreshold();
}

/*************** TREE CHANGED(ITERATOR) ***************/
void QmitkThresholdComponent::TreeChanged(mitk::DataTreeIteratorBase* it)
{
  if(m_GUI != NULL)
  {
    SetDataTreeIterator(it);
  }
}

/***************       CONNECTIONS      ***************/
void QmitkThresholdComponent::CreateConnections()
{
  if ( m_GUI )
  {
    connect( (QObject*)(m_GUI->GetTreeNodeSelector()), SIGNAL(activated(const mitk::DataTreeFilter::Item *)), (QObject*) this, SLOT(ImageSelected(const mitk::DataTreeFilter::Item *)));
    connect( (QObject*)(m_GUI->GetThresholdInputSlider()), SIGNAL(sliderReleased()), (QObject*) this, SLOT(ThresholdSliderChanged()));
    connect( (QObject*)(m_GUI->GetThresholdInputNumber()), SIGNAL(returnPressed()), (QObject*) this, SLOT(ThresholdValueChanged()));    
    connect( (QObject*)(m_GUI->GetShowThresholdGroupBox()), SIGNAL(toggled(bool)), (QObject*) this, SLOT(ShowThreshold(bool)));     
    connect( (QObject*)(m_GUI->GetThresholdFinderGroupBox()), SIGNAL(toggled(bool)), (QObject*) this, SLOT(ShowThresholdContent(bool)));     
    connect( (QObject*)(m_GUI->GetThresholdSelectDataGroupBox()), SIGNAL(toggled(bool)), (QObject*) this, SLOT(ShowImageContent(bool)));      
  }
}

/***************     IMAGE SELECTED     ***************/
void QmitkThresholdComponent::ImageSelected(const mitk::DataTreeFilter::Item * imageIt)
{
  m_SelectedImage = imageIt;
  mitk::DataTreeFilter::Item* currentItem(NULL);
  if(m_GUI)
  {
    if(mitk::DataTreeFilter* filter = m_GUI->GetTreeNodeSelector()->GetFilter())
    {
      if(imageIt)
      {
        currentItem = const_cast <mitk::DataTreeFilter::Item*> ( filter->FindItem( imageIt->GetNode() ) );
      }
    }
  }
  if(currentItem)
  {
    currentItem->SetSelected(true);
  }
    if(m_GUI != NULL)
  {

    for(unsigned int i = 0;  i < m_AddedChildList.size(); i++) 
    {
      m_AddedChildList[i]->ImageSelected(m_SelectedImage);
    }
  }
  m_Node = const_cast<mitk::DataTreeNode*>(m_SelectedImage->GetNode());
  TreeChanged();
  SetSliderRange();
}

/*************** CREATE CONTAINER WIDGET **************/
QWidget* QmitkThresholdComponent::CreateContainerWidget(QWidget* parent)
{
  m_GUI = new QmitkThresholdComponentGUI(parent);
  m_GUI->GetTreeNodeSelector()->SetDataTree(GetDataTreeIterator());
  CreateThresholdImageNode();
  return m_GUI;
}

/***************        ACTIVATED       ***************/
void QmitkThresholdComponent::Activated()
{
  m_Active = true;
  ShowThreshold();
}

///***************  CREATE NEW IMAGE NODE  **************/
void QmitkThresholdComponent::CreateThresholdImageNode()
{
  if (m_Node)
  {
    m_ThresholdImageNode = mitk::DataTreeNode::New();
    mitk::StringProperty::Pointer nameProp = new mitk::StringProperty("threshold image" );
    m_ThresholdImageNode->SetProperty( "name", nameProp );
    m_ThresholdImageNode->SetData(m_Node->GetData());
    m_ThresholdImageNode->SetColor(0.0,1.0,0.0);
    m_ThresholdImageNode->SetOpacity(.25);
    int layer = 0;
    m_Node->GetIntProperty("layer", layer);
    m_ThresholdImageNode->SetIntProperty("layer", layer+1);
    m_ThresholdImageNode->SetLevelWindow(mitk::LevelWindow(m_GUI->GetNumberValue(),1));
    
    mitk::DataTreeIteratorClone iteratorClone = m_DataTreeIterator;
    while ( !iteratorClone->IsAtEnd() )
    {
      mitk::DataTreeNode::Pointer node = iteratorClone->Get();
      if (  node == m_Node )
      {
        iteratorClone->Add(m_ThresholdImageNode);
      }
      ++iteratorClone;
    m_ThresholdImageNode->SetProperty("visible", new mitk::BoolProperty((false)) );
    }
  }
}

///***************      SHOW THRESHOLD     **************/
void QmitkThresholdComponent::ShowThreshold(bool)
{
  if(m_Active == true)
  {
    m_ThresholdImageNode->SetProperty("visible", new mitk::BoolProperty((m_GUI->GetShowThresholdGroupBox()->isChecked())) );
  }
  m_GUI->GetThresholdValueContent()->setShown(m_GUI->GetShowThresholdGroupBox()->isChecked());
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}


///***************  SHOW THRESHOLD CONTENT **************/
void QmitkThresholdComponent::ShowThresholdContent(bool)
{
  m_GUI->GetShowThresholdGroupBox()->setShown(m_GUI->GetThresholdFinderGroupBox()->isChecked());
  m_GUI->GetThresholdSelectDataGroupBox()->setShown(m_GUI->GetThresholdFinderGroupBox()->isChecked());
  ShowThreshold();
}

///***************    SHOW IMAGE CONTENT   **************/
void QmitkThresholdComponent::ShowImageContent(bool)
{
  m_GUI->GetImageContent()->setShown(m_GUI->GetThresholdSelectDataGroupBox()->isChecked());
  m_GUI->GetTreeNodeSelector()->setShown(m_GUI->GetThresholdSelectDataGroupBox()->isChecked());
}

///*************** THRESHOLD VALUE CHANGED **************/
//By Slider
void QmitkThresholdComponent::ThresholdSliderChanged()
{
  int value = m_GUI->GetThresholdInputSlider()->value();
  if (m_ThresholdImageNode)
  {
    m_ThresholdImageNode->SetLevelWindow(mitk::LevelWindow(value,1));
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  }

  m_GUI->GetThresholdInputNumber()->setText(QString::number(value)); 
}

///*************** THRESHOLD VALUE CHANGED **************/
//By LineEdit
void QmitkThresholdComponent::ThresholdValueChanged( )
{
  int value = m_GUI->GetNumberValue();
  if (m_ThresholdImageNode)
  {
    m_ThresholdImageNode->SetLevelWindow(mitk::LevelWindow(value,1));
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  }
  m_GUI->GetThresholdInputSlider()->setValue(value);
}


///***************     SET SLIDER RANGE    **************/
void QmitkThresholdComponent::SetSliderRange()
{
  if(m_GUI->GetThresholdFinderGroupBox()->isChecked()==true)
  {
    mitk::Image* currentImage = dynamic_cast<mitk::Image*>(m_ThresholdImageNode->GetData());
    m_GUI->GetThresholdInputSlider()->setMinValue(currentImage->GetScalarValueMin());
    m_GUI->GetThresholdInputSlider()->setMaxValue(currentImage->GetScalarValueMax());
  }
}

